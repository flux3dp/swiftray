#pragma once

#include <constants.h>
#include <document.h>
#include <layer.h>
#include <shape/bitmap-shape.h>
#include <shape/group-shape.h>
#include <shape/path-shape.h>
#include <toolpath_exporter/generators/base-generator.h>
#include <toolpath_exporter/generators/fcode-generator.h>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QMutex>
#include <QPainter>
#include <QProgressDialog>
#include <QVector2D>
#include <QVector3D>
#include <bitset>
#include <opencv2/flann.hpp>

struct NozzleSettings {
  float voltage = 9.0;
  float voltage_default = 9.0;
  float pulse_width = 2.0;
  float pulse_width_default = 2.0;
  int saturation = 3;
  int DPI = 600;
  int ink_catridge_count = 1;
  int ink_type = 0;
  int nozzle_select = 0;
  int spray_time = 0;
  int ink_exchange = 0;
  int h_gap_ink1_ink2 = 0;
  int v_gap_ink1_ink2 = 0;
  int h_gap_ink2_ink3 = 0;
  int v_gap_ink2_ink3 = 0;
  int h_gap_ink3_ink4 = 0;
  int v_gap_ink3_ink4 = 0;
};

struct CurveEngravingSettings {
  QRectF bbox;
  QPointF gap;
  QList<QVector3D> points;
  cv::Ptr<cv::flann::Index> kdTree;
};

struct Config {
  // mm/min
  float min_speed = 3;
  float travel_speed = 7500; // default val = 7500 in ghost, 12000 in client
  float a_travel_speed = 2000;
  float path_travel_speed = 7500; // default val = 3600 for ador, 7500 for others
  float prespray_speed = 1800;
  float prespray_travel_speed = 7500;
  // mm^2/s
  float path_acc = std::nanf("");
  float padding_acc = 4000;
  // mm
  float min_engraving_padding = std::nanf("");
  float min_printing_padding = std::nanf("");
  float spinning_axis_coord = -1;
  float z_offset = 0;
  float blade_radius = 0;
  float loop_compensation = 0;
  float workarea_clip[4] = {0, 0, 0, 0}; // inward offset; top, right, bottom, left
  QPointF precut_at;
  QPointF diode_offset;
  QPointF job_origin;
  QMap<int, QPointF> module_offsets;
  QRectF prespray;
  // px
  int printing_top_padding = std::nanf("");
  int printing_bot_padding = std::nanf("");
  int fg_pwm_limit = 1500;
  // px/mm
  float dpmm_x = 10;
  float dpmm_y = 10;
  float dpmm_printing = 300 / 25.4;
  float dpmm_preview;
  // fountion on/off
  bool enable_pwm = true;
  bool enable_precut = false;
  bool enable_diode = false;
  bool enable_autofocus = false;
  bool enable_custom_backlash = false;
  bool enable_fast_gradient = false;
  bool enable_mock_fast_gradient = false;
  bool enable_multipass_compensation = false;
  bool enable_vector_speed_constraint = false;
  bool enable_relative_z_move = false;
  bool is_one_way_printing = false;
  bool is_diode_one_way_engraving = false;
  bool is_reverse_engraving = false;

  char print_modes[2] = {0, 0};
};

class ToolpathExporterFcode : public QObject {
  Q_OBJECT

 public:
  enum class HardwareType { beamo, Beambox, BeamboxPro, HEXA, Ador, BB2 };

  ToolpathExporterFcode(QTransform move_translate,
                        int dpi,
                        const QJsonObject* param,
                        const QString* thumbnail) noexcept
      : move_translate_(move_translate) {
    qInfo() << "ToolpathExporterFcode init";
    parseParam(param);
    setDpi(dpi);

    if (is_v2_) {
      if (is_rotary_task_ || with_custom_origin_) {
        gen = std::make_shared<FCodeGeneratorV2>(thumbnail, 4, with_custom_origin_);
      } else {
        gen = std::make_shared<FCodeGeneratorV2>(thumbnail, 3, with_custom_origin_);
      }
    } else {
      gen = std::make_shared<FCodeGeneratorV1>(thumbnail, with_custom_origin_);
    }
    gen_ = gen.get();

    setTravelSpeed(config_.travel_speed);
  }

  std::string toString() { return gen_->to_string(); };

  void save(QDataStream* out) {
    out->writeRawData(toString().c_str(), gen_->total_length());
  }

  float getTimeCost() { return gen_->get_time_cost(); }

  bool convertStack(const QList<LayerPtr>& layers,
                    QProgressDialog* dialog = nullptr);

 private:
  void parseParam(const QJsonObject* paramPtr) {
    QJsonObject param = *paramPtr;
    if (param.contains("job_origin")) {
      with_custom_origin_ = true;
      config_.job_origin = QPointF(param["job_origin"].toArray()[0].toDouble(),
                                   param["job_origin"].toArray()[1].toDouble());
    }
    float spinning_axis_coord = param["spin"].toDouble();
    if (spinning_axis_coord > 0) {
      is_rotary_task_ = true;
      config_.spinning_axis_coord = spinning_axis_coord / canvas_mm_ratio - config_.job_origin.y();
      rotary_y_ratio_ = param["rotary_y_ratio"].toDouble(1);
    }

    QJsonObject workarea = param["workarea"].toObject();
    int width = workarea["width"].toInt();
    int height = workarea["height"].toInt();
    QString hardware = param["hardware_name"].toString();
    float default_path_travel_speed = 7500;
    float default_path_acc = std::nanf("");
    if (hardware == "beamo") {
      hardware_ = HardwareType::beamo;
    } else if (hardware == "pro") {
      hardware_ = HardwareType::BeamboxPro;
    } else if (hardware == "hexa") {
      hardware_ = HardwareType::HEXA;
      config_.fg_pwm_limit = 0;
      config_.enable_relative_z_move = true;
    } else if (hardware == "ado1") {
      hardware_ = HardwareType::Ador;
      is_v2_ = true;
      with_module_ = true;
      config_.fg_pwm_limit = 0;
      config_.enable_relative_z_move = true;
      if (is_rotary_task_) {
        height += 378.2;
      }
      default_path_travel_speed = 3600;
      default_path_acc = 500;
      if (param.contains("prespray")) {
        QJsonArray prespray_arr = param["prespray"].toArray();
        config_.prespray =
            QRectF(prespray_arr[0].toDouble(), prespray_arr[1].toDouble(),
                   prespray_arr[2].toDouble(), prespray_arr[3].toDouble());
      }
    } else if (hardware == "fbb2") {
      hardware_ = HardwareType::BB2;
      is_v2_ = true;
      config_.fg_pwm_limit = 0;
      config_.enable_relative_z_move = true;
      default_path_acc = 1000;
    } else {
      // default beambox
      hardware_ = HardwareType::Beambox;
    }
    work_area_mm_ = QSizeF(width, height);
    config_.dpmm_preview = 500.0 / width;

    if (with_module_) {
      QJsonObject offset_dict = param["mof"].toObject();
      for (QString module_key : offset_dict.keys()) {
        QJsonArray offset = offset_dict[module_key].toArray();
        config_.module_offsets[module_key.toInt()] =
            QPointF(offset[0].toDouble(), offset[1].toDouble());
      }
    }

    if (param.contains("diode")) {
      config_.enable_diode = true;
      QJsonArray diode_offset = param["diode"].toArray();
      config_.diode_offset =
          QPointF(diode_offset[0].toDouble(), diode_offset[1].toDouble());
    }
    config_.enable_autofocus = param["af"].toBool();
    config_.enable_custom_backlash = param["cbl"].toBool();
    config_.enable_fast_gradient = param["fg"].toBool();
    config_.enable_mock_fast_gradient = param["mfg"].toBool();
    config_.enable_pwm = !param["no_pwm"].toBool();
    config_.enable_multipass_compensation = param["mpc"].toBool();
    config_.enable_vector_speed_constraint = param["vsc"].toBool();
    config_.is_one_way_printing = param["owp"].toBool();
    config_.is_diode_one_way_engraving = param["diode_owe"].toBool();
    config_.is_reverse_engraving = param["rev"].toBool();
    config_.min_speed = param["min_speed"].toDouble(3);
    config_.travel_speed = param["ts"].toDouble(7500);
    config_.a_travel_speed = param["ats"].toDouble(2000);
    config_.path_travel_speed = param["pts"].toDouble(default_path_travel_speed);
    config_.path_acc = param["path_acc"].toDouble(default_path_acc);
    config_.padding_acc = param["acc"].toDouble(4000);
    config_.min_engraving_padding = param["mep"].toDouble(std::nanf(""));
    config_.min_printing_padding = param["mpp"].toDouble(std::nanf(""));
    config_.z_offset = param["z_offset"].toDouble(0);
    config_.blade_radius = param["blade"].toDouble();
    if (config_.blade_radius > 0) {
      with_blade_ = true;
      if (param.contains("precut")) {
        config_.enable_precut = true;
        config_.precut_at = QPointF(param["precut"].toArray()[0].toDouble(),
                                    param["precut"].toArray()[1].toDouble());
      }
    }
    config_.loop_compensation = param["loop_compensation"].toDouble() / canvas_mm_ratio;
    config_.printing_top_padding = param["ptp"].toInt(10);
    config_.printing_bot_padding = param["pbp"].toInt(10);
    if (param.contains("nv")) {
      nozzle_settings.voltage = param["nv"].toDouble();
    }
    if (param.contains("npw")) {
      nozzle_settings.pulse_width = param["npw"].toDouble();
    }

    QJsonArray clip = param["mask"].toArray();
    if (clip.size() == 4) {
      for (int i = 0; i < 4; i++) {
        config_.workarea_clip[i] = clip[i].toDouble();
      }
    }

    QJsonObject curve_obj = param["curve_engraving"].toObject();
    if (!curve_obj.isEmpty()) {
      qInfo() << "Set curve engraving data";
      QJsonObject bbox = curve_obj["bbox"].toObject();
      float box_left = bbox["x"].toDouble();
      config_.workarea_clip[3] = qMax(config_.workarea_clip[3], box_left);
      float box_top = bbox["y"].toDouble();
      config_.workarea_clip[0] = qMax(config_.workarea_clip[0], box_top);
      float box_width = bbox["width"].toDouble();
      float box_right = box_left + box_width;
      config_.workarea_clip[1] = qMax(config_.workarea_clip[1], box_right);
      float box_height = bbox["height"].toDouble();
      float box_bottom = box_top + box_height;
      config_.workarea_clip[2] = qMax(config_.workarea_clip[2], box_bottom);

      QJsonArray points = curve_obj["points"].toArray();
      int point_size = points.size();
      if (point_size >= 3) {
        is_3d_task_ = true;
        // add 0.01 to avoid clipping the boundary
        float space = 0.01;
        curve_settings.bbox = QRectF(box_left - space, box_top - space, box_width + 2 * space, box_height + 2 * space);
        QJsonArray gap = curve_obj["gap"].toArray();
        curve_settings.gap = QPointF(gap[0].toDouble(), gap[1].toDouble());

        cv::Mat point_mat = cv::Mat::zeros(point_size, 2, CV_32F);
        for (int i = 0; i < point_size; i++) {
          QJsonArray point = points[i].toArray();
          float x = point[0].toDouble();
          float y = point[1].toDouble();
          float z = point[2].toDouble();
          curve_settings.points.append(QVector3D(x, y, z));
          point_mat.at<float>(i, 0) = x;
          point_mat.at<float>(i, 1) = y;
        }
        curve_settings.kdTree = cv::makePtr<cv::flann::Index>(point_mat, cv::flann::KDTreeIndexParams(1));
      }
    }
  }

  void setDpi(int dpi) {
    qInfo() << "Set dpi" << dpi;
    if (dpi < 200) {
      // low: 127
      config_.dpmm_x = config_.dpmm_y = 5;
      config_.print_modes[0] = 'P', config_.print_modes[1] = 'L';
    } else if (dpi < 500) {
      // medium: 254
      config_.dpmm_x = config_.dpmm_y = 10;
      config_.print_modes[0] = 'Q', config_.print_modes[1] = 'M';
    } else if (dpi < 1000) {
      // high: 508
      config_.dpmm_x = config_.dpmm_y = 20;
      config_.print_modes[0] = 'R', config_.print_modes[1] = 'H';
    } else {
      // ultra: 1016
      config_.dpmm_x = 20, config_.dpmm_y = 50;
    }

    transform_laser_ = QTransform::fromScale(config_.dpmm_x / canvas_mm_ratio,
                                             config_.dpmm_y / canvas_mm_ratio);
    transform_printing_ =
        QTransform::fromScale(config_.dpmm_printing / canvas_mm_ratio,
                              config_.dpmm_printing / canvas_mm_ratio);
    transform_preview_laser_ =
        QTransform::fromScale(config_.dpmm_preview / config_.dpmm_x,
                              config_.dpmm_preview / config_.dpmm_y);
    transform_preview_printing_ =
        QTransform::fromScale(config_.dpmm_preview / config_.dpmm_printing,
                              config_.dpmm_preview / config_.dpmm_printing);
  }

  void setTransform(QTransform transform = QTransform()) {
    global_transform_ =
        transform * move_translate_ *
        (is_printing_layer_ ? transform_printing_ : transform_laser_);
  }
  QTransform getPreviewTransform() {
    return is_printing_layer_ ? transform_preview_printing_
                              : transform_preview_laser_;
  }
  float dpmm_x() {
    return is_printing_layer_ ? config_.dpmm_printing : config_.dpmm_x;
  }
  float dpmm_y() {
    return is_printing_layer_ ? config_.dpmm_printing : config_.dpmm_y;
  }
  QPointF getPointInMM(QPointF point) {
    return QPointF(px2mm(point.x(), true), px2mm(point.y())) - module_offset_;
  }
  qreal getXValInMM(qreal val, bool is_reverse = false, bool check_negative = false) {
    // Add 1 px width to indicate the right side of the pixel when reverse
    qreal x = px2mm(val, true);
    if (check_negative)
      x = qMax(x, float(0));
    x -= module_offset_.x();
    if (!is_reverse)
      x += backlash_;
    return x;
  }
  qreal getYValInMM(qreal val) {
    return px2mm(val) - module_offset_.y();
  }
  float px2mm(int px, bool is_x = false) {
    return float(px) / (is_x ? dpmm_x() : dpmm_y());
  }
  float mm2px(float mm, bool is_x = false) {
    return mm * (is_x ? dpmm_x() : dpmm_y());
  }
  void setTravelSpeed(float feedrate = std::nanf("")) {
    if (!std::isnan(feedrate)) {
      travel_speed_ = feedrate;
    }
  }
  void setPathAcceleration(float x = std::nanf(""),
                           float y = std::nanf(""),
                           float z = std::nanf(""),
                           float a = std::nanf("")) {
    int flags = 0;
    if (!std::isnan(x)) {
      flags |= FCodeGenerator::move_flag_X;
    }
    if (!std::isnan(y)) {
      flags |= FCodeGenerator::move_flag_Y;
    }
    if (!std::isnan(z)) {
      flags |= FCodeGenerator::move_flag_Z;
    }
    if (!std::isnan(a)) {
      flags |= FCodeGenerator::move_flag_A;
    }
    gen_->set_path_acceleration(flags, x, y, z, a);
  }
  float getCurveEngravingHeight();

  void updateLayerParam();
  void updateOffset();
  void updateClip();

  void convertLaserLayer();
  void convertPrintingLayer();

  bool convertShape(const ShapePtr& shape, bool from_group = false);
  bool convertGroup(const GroupShape* group);
  void convertBitmap(const BitmapShape* bmp);
  void convertPath(const PathShape* path);

  void sortPolygons();

  void outputLayerPathFcode();
  int getPointPosition(QPointF point);
  void getIntersectPoint(QLineF line, int position, QPointF* point);
  void handlePathWalk(QPointF point, bool should_emit);

  void outputBitmapFcode(bool pwm_engraving = false, int downsample = 5);
  bool rasterBitmap(const QImage& layer_image, QRect bbox, bool pwm_engraving);
  bool rasterLine(const uchar* data_ptr,
                  int left_bound,
                  int right_bound,
                  int y,
                  bool reverse_raster_dir);
  bool rasterLineHighSpeed(const uchar* data_ptr,
                           int left_bound,
                           int right_bound,
                           int y,
                           bool reverse_raster_dir);
  bool rasterLineHighSpeedPwm(const uchar* data_ptr,
                              int left_bound,
                              int right_bound,
                              int y,
                              bool reverse_raster_dir);

  void outputLayerPrintingFcode(float halftone_multiplier);
  QByteArray generateNozzleSettingPayload(int saturation = 3,
                                          bool use_default = false);
  void sliceBox(QList<QList<QList<int>>>* sliced_boxes,
                QRect box,
                int multipass = 1);

  std::tuple<QRect, QRect> getPresprayBbox();
  void writeSimpleFilledTaskCode(QRect bbox, int offset_y = 0);
  void writeCatridgeTaskCode(QRect bbox);

  void writePreviewImage();

  QImage imageBinarize(QImage* src, int threshold);
  void clearWhite(QImage* src);
  void clearTransparent(QImage* src);
  QVector<QRect> getBoundingBoxes(QImage* src,
                                  int merge_offset_x = 0,
                                  int merge_offset_y = 0,
                                  float downsample = 1);

  void pause(bool to_standby_position);
  void moveZ(float z);
  void travel(float x, float y, bool force_y = false, float s = std::nanf(""));
  void travel(QPointF position, bool force_y = false, float s = std::nanf(""));
  void moveto(float feedrate = std::nanf(""),
              float x = std::nanf(""),
              float y = std::nanf(""),
              float z = std::nanf(""),
              float s = std::nanf(""),
              bool force_y = false,
              bool is_travel = false);
  void moveto_(float feedrate,
               float x,
               float y,
               float z,
               float s,
               bool force_y,
               bool is_travel);

  // white = 255 = no emit, black = 0 = emit
  const int white_val = 255;
  // for pwm, val < pwm_threshold = emit
  const int pwm_threshold = 254;
  const float canvas_mm_ratio = 10.0;
  const int printing_slice_width = 5160;
  const int printing_slice_height = 150;
  const float halftone_smoother = 1.0;
  const float am_density = 2.0;
  const QMap<QString, float> am_angles = {{"cyan", 22.5},
                                          {"magenta", 22.5},
                                          {"yellow", 22.5},
                                          {"white", 75},
                                          {"black", 52.5}};
  const QMap<QString, QList<int>> am_color_curves = {
      {"cyan", {0, 47, 107, 159, 255}},
      {"magenta", {0, 43, 115, 169, 255}},
      {"yellow", {0, 59, 115, 175, 255}},
      {"black", {0, 47, 111, 143, 255}}};
  const QMap<QString, QList<int>> fm_color_curves = {
      {"cyan", {0, 15, 47, 95, 255}},
      {"magenta", {0, 15, 47, 191, 255}},
      {"yellow", {0, 12, 37, 143, 255}},
      {"black", {0, 15, 47, 79, 255}}};

  std::shared_ptr<FCodeGenerator> gen;
  FCodeGenerator* gen_;

  // Canvas
  QMutex polygons_mutex_;
  QList<QPolygonF> layer_polygons_;
  QList<ShapePtr> layer_bitmaps_;
  std::unique_ptr<QPainter> layer_painter_;
  QPixmap laser_bitmap_;
  QPixmap printing_bitmap_;
  std::unique_ptr<QPainter> preview_painter_;
  QPixmap preview_bitmap_;
  QRectF bitmap_dirty_area_ = QRectF(); // px according to current layer dpi

  // Basic config
  Config config_;
  HardwareType hardware_ = HardwareType::Beambox;
  NozzleSettings nozzle_settings;
  CurveEngravingSettings curve_settings;
  bool is_v2_ = false;
  bool is_rotary_task_ = false;
  bool is_3d_task_ = false;
  bool with_blade_ = false;
  bool with_module_ = false;
  bool with_custom_origin_ = false;

  QSizeF work_area_mm_;
  QTransform transform_laser_;
  QTransform transform_printing_;
  QTransform transform_preview_laser_;
  QTransform transform_preview_printing_;
  QTransform move_translate_;

  // Updated for each layer
  LayerPtr current_layer_;
  bool enable_bidirection_;
  float layer_speed_sec_;  // mm/s
  float layer_speed_;      // mm/min
  float path_speed_;       // mm/min
  float backlash_ = 0;
  float padding_mm_;
  int padding_px_;
  int layer_module_;
  bool is_printing_layer_;
  float focus_adjust_ = 0;
  float focus_step_ = 0;
  bool has_focus_adjust_;
  float pwm_scale_ = 1;
  QPointF module_offset_;
  float rotary_y_offset_ = 0;  // mm
  QString layer_color_;
  QString submodule_color_ = "None";
  QRect clip_area_;         // px;
  QLineF border_lines_[4];  // px; top, right, bottom, left

  // Updated during processing
  bool is_handling_bitmap_ = false;
  bool is_a_mode_ = false;
  bool rotary_wait_move_ = false;
  bool did_home_z_ = false;
  bool with_print_task_ = false;
  bool disable_rotary_ = true;
  float travel_speed_ = 12000;
  float rotary_y_ratio_ = 1;  // force set to 1 in post script
  QTransform global_transform_;
  // For blade: blade position: current_xy - blade_radius * (current_vector / |current_vector|)
  QPointF current_xy_ = QPointF(0, 0); // mm position of control point (not blade)
  QVector2D current_vector_ = QVector2D(0, 0); // vector of cutting movement
  // For 3d curve
  float curve_x_ = 0;
  float curve_y_ = 0;
};
