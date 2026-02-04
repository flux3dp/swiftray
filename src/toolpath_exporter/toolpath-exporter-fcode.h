// Update to
// Ghost: 3fe4630d89939d257e291c3a3e01659833e44a2c
// Client: b1255ac0eef3770c36c0a90c2f50b53e15feaab7

#pragma once

#include <constants.h>
#include <document.h>
#include <layer.h>
#include <shape/bitmap-shape.h>
#include <shape/group-shape.h>
#include <shape/path-shape.h>
#include <toolpath_exporter/generators/base-generator.h>
#include <toolpath_exporter/generators/fcode-generator.h>
#include <toolpath_exporter/generators/interpolation.cpp>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QMutex>
#include <QPainter>
#include <QProgressDialog>
#include <QVector2D>
#include <bitset>
#include <cmath>
#include "toolpath-utils.h"

struct CurveEngravingSettings {
  QRectF bbox;
  QPointF gap;
  float safe_height;
  CloughTocher2DInterpolator interpolator;
};

struct Config {
  // mm/min
  float z_speed = 7.5; // for time estimated; bb2 = 5.16
  float min_speed = 3;
  float travel_speed = 7500; // default val = 7500 in ghost, 12000 in client
  float a_travel_speed = 2000;
  float path_travel_speed = 7500; // default val = 3600 for ador, 7500 for others
  float prespray_speed = 1800;
  float prespray_travel_speed = 7500;
  float vector_speed_constraint = 0;
  float z_premove_speed = 0;
  // mm/s
  float curve_speed_constraint = 0;
  // mm^2/s
  float padding_acc = 4000;
  // mm
  float min_engraving_padding = NAN;
  float min_printing_padding = NAN;
  float spinning_axis_coord = -1;
  float z_offset = 0;
  float loop_compensation = 0;
  float workarea_clip[4] = {0, 0, 0, 0}; // inward offset; top, right, bottom, left
  float z_premove_x = 0;
  float z_premove_y = 0;
  float z_premove_z = 0;
  QPointF diode_offset;
  QPointF job_origin;
  QMap<int, QPointF> module_offsets;
  QRectF prespray;
  // px
  int printing_top_padding = 0;
  int printing_bot_padding = 0;
  int fg_pwm_limit = 0;
  // px/mm
  float dpmm_x = 10;
  float dpmm_y = 10;
  float dpmm_printing = 300 / 25.4;
  float dpmm_preview;
  // fountion on/off
  bool enable_pwm = true;
  bool enable_diode = false;
  bool enable_autofocus = false;
  bool enable_custom_backlash = false;
  bool enable_fast_gradient = false;
  bool enable_mock_fast_gradient = false;
  bool enable_multipass_compensation = false;
  bool enable_rotary_z_move = false;
  bool enable_segmentation = false;
  bool is_one_way_printing = false;
  bool is_diode_one_way_engraving = false;
  bool is_reverse_engraving = false;
  // founction based on hardware
  bool support_rel_z_move = false;
  bool support_modules = false;
  bool support_rotary_z_motion = false;

  char print_modes[2] = {0, 0};
  QJsonObject fill_acc = {};
  QJsonObject path_acc = {};
};

struct MoveArgs {
  float f = NAN;
  float x = NAN;
  float y = NAN;
  float z = NAN;
  float a = NAN;
  float s = NAN;
  bool force_y = false;
  bool is_travel = false;
};

class ToolpathExporterFcode : public QObject {
  Q_OBJECT

 public:
  enum class HardwareType { beamo, Beambox, BeamboxPro, HEXA, Ador, BB2, RF };

  ToolpathExporterFcode(QTransform move_translate,
                        int dpi,
                        const QJsonObject* param,
                        const QString* thumbnail) noexcept;
  std::string toString();
  void save(QDataStream* out);
  float getTimeCost();
  QJsonObject getMetadata();
  bool convertStack(const QList<LayerPtr>& layers,
                    QProgressDialog* dialog = nullptr);

Q_SIGNALS:
  void progressChanged(int value);

public Q_SLOTS:
  void handleCancel();

 private:
  void parseParam(const QJsonObject* paramPtr);
  void setDpi(int dpi) {
    // deprecated
  }

  void setTransform(QTransform transform = QTransform());
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
  float px2mm(float px, bool is_x = false) {
    return float(px) / (is_x ? dpmm_x() : dpmm_y());
  }
  float mm2px(float mm, bool is_x = false) {
    return mm * (is_x ? dpmm_x() : dpmm_y());
  }
  void setTravelSpeed(float feedrate = NAN) {
    if (!std::isnan(feedrate)) {
      travel_speed_ = feedrate;
    }
  }
  void setAcceleration(float x = NAN, float y = NAN, float z = NAN, float a = NAN) {
    // deprecated
  }

  void updateLayerParam();
  void updateOffset();
  void updateClip();

  void convertLaserLayer();
  void convertPrintingLayer();

  bool convertShape(const ShapePtr& shape, bool from_group = false);
  bool convertGroup(const GroupShape* group);
  void convertBitmap(const BitmapShape* bmp);
  void convertPath(const PathShape* path);

  void outputLayerPathFcode();
  void handlePathWalk(QPointF point, bool should_emit);

  void outputBitmapFcode(bool pwm_engraving = false);
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

  void clearWhite(QImage* src, QRect dirty_area);
  void clearTransparent(QImage* src);
  QVector<QRect> getBoundingBoxes(QImage* src,
                                  int merge_offset_x = 0,
                                  int merge_offset_y = 0,
                                  int downsample = 1);

  void pause(bool to_standby_position);

  // Move related functions
  void updateMovetoPipeline();
  void moveZ(float z);
  void travel(float x, float y, bool force_y = false, float s = NAN);
  void travel(QPointF position, bool force_y = false, float s = NAN);
  void moveto(float feedrate = NAN,
              float x = NAN,
              float y = NAN,
              float z = NAN,
              float a = NAN,
              float s = NAN,
              bool force_y = false,
              bool is_travel = false);
  void pipelineMoveto(int idx, MoveArgs args);
  void rotaryMotionGenerator(MoveArgs args,
                             std::function<void(MoveArgs args)> callback);
  void curveEngravingMotionGenerator(
      MoveArgs args,
      std::function<void(MoveArgs args)> callback);
  void zPremoveMotionGenerator(MoveArgs args,
                               std::function<void(MoveArgs args)> callback);
  void _moveto(MoveArgs args);

  void onProgressChanged(double value, bool absolute);

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

  ToolpathProcessor proc;
  std::shared_ptr<FCodeGenerator> gen;
  FCodeGenerator* gen_;

  // Canvas
  QMutex polygons_mutex_;
  QList<QPolygonF> layer_polygons_;
  QList<ShapePtr> layer_bitmaps_;
  std::unique_ptr<QPainter> layer_painter_;
  QImage laser_bitmap_;
  QImage printing_bitmap_;
  std::unique_ptr<QPainter> preview_painter_;
  QImage preview_bitmap_;
  QRectF bitmap_dirty_area_ = QRectF(); // px according to current layer dpi

  // Basic config
  Config config_;
  HardwareType hardware_ = HardwareType::Beambox;
  NozzleSettings nozzle_settings;
  CurveEngravingSettings curve_settings;
  int magic_number_ = 0;
  bool is_gcode_ = false;
  bool is_v2_ = false;
  bool is_rotary_task_ = false;
  bool is_3d_task_ = false;
  bool with_custom_origin_ = false;
  PathUtils path_utils_;

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
  float curve_z_limit_ = 0;
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

  // Updated during processing
  bool is_handling_main_work_ = false;
  bool is_handling_3d_work_ = false;
  bool is_handling_bitmap_ = false;
  bool is_a_mode_ = false;
  bool rotary_wait_move_ = false;
  bool did_home_z_ = false;
  bool with_print_task_ = false;
  bool disable_rotary_ = true;
  float travel_speed_ = 12000;
  float rotary_y_ratio_ = 1;  // force set to 1 in post script
  QTransform global_transform_;
  std::vector<void(ToolpathExporterFcode::*)(MoveArgs args, std::function<void(MoveArgs args)> callback)> moveto_pipeline_functions_;
  // For 3d curve
  float curve_started_ = false;
  float cur_x_ = 0;
  float cur_y_ = 0;
  float cur_z_ = 0;
  float cur_f_ = 12000;
  float target_f_ = NAN;
  // Metadata
  float min_x_ = NAN;
  float max_x_ = NAN;
  float min_y_ = NAN;
  float max_y_ = NAN;
  float min_z_ = NAN;
  float max_z_ = NAN;
  // Task progress
  QProgressDialog* dialog_ = nullptr;
  bool cancelled_ = false;
  int total_layer_cnt_ = 1;
  int processed_layer_cnt_ = 0;
  int total_repeat_times_ = 1;
  int processed_repeat_times_ = 0;
  int element_cnt_[2] = {0, 0}; // 0 Path, 1 Filled Path (Bitmap)
  int total_element_cnt_ = 0;
  double bitmap_progress_unit_ = 0;
  double current_progress_ = 0; // progress within current repeat
  int progress_ = 0;
};
