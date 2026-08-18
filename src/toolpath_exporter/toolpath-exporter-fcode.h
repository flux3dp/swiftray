// Updated to
// Ghost: b10a72e5e59c5f03b7e214953557903af141d611
// Client: 3b285b419b2852b4265f4f1508bacb5b95b1a4c8

#pragma once

#include "toolpath-utils.h"
#include "toolpath-exporter-constants.h"
#include "toolpath_exporter/factories/base-factory.h"
#include "toolpath_exporter/factories/laser-path.h"
#include "toolpath_exporter/generators/fcode-generator.h"
#include "toolpath_exporter/macros/base-macros.h"
#include "layer.h"
#include "shape/bitmap-shape.h"
#include "shape/group-shape.h"
#include "shape/path-shape.h"
#include <QImage>
#include <QJsonArray>
#include <QJsonObject>
#include <QProgressDialog>
#include <QVector>

enum class ConvertTarget {
  ALL,         // printer and uv
  NON_BITMAP,  // laser first pass
  BITMAP_ONLY  // laser second pass
};

enum class ProgressType {
  PRE_TASK,
  LAYERS,
  PRE_LAYER,
  LAYER_PATH,
  LAYER_BITMAP,
  POST_LAYER,
  POST_TASK
};

struct Config {
  // mm/min
  float min_speed = 3;
  float travel_speed = 7500;  // default val = 7500 in ghost, 12000 in client
  float a_travel_speed = 2000;
  float path_travel_speed =
      7500;  // default val = 3600 for ador, 7500 for others
  float vector_speed_limit = 0;
  float curve_speed_limit = 0;
  // mm^2/s
  float padding_acc = 4000;
  float z_acc = NAN;
  AccelerationData fill_acc = {};
  AccelerationData path_acc = {};
  // mm
  float min_engraving_padding = NAN;
  float min_printing_padding = NAN;
  float spinning_axis_coord_mm = -1;
  float z_offset = 0;
  float loop_compensation = 0;
  float engraving_erode = 0;
  InwardRect workarea_clip = {};
  QPointF diode_offset;
  QPointF job_origin;
  QPointF home_pos;
  QMap<LayerModule, QPointF> module_offsets;
  QRectF prespray;
  // px
  int printing_top_padding = -1;  // use -1 (invalid value) to indicate not set
  int printing_bot_padding = -1;
  int printing_slice_width = -1;
  int printing_slice_height = -1;
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
  bool enable_s_curve = false;
  bool is_one_way_printing = false;
  bool is_diode_one_way_engraving = false;
  bool is_reverse_engraving = false;
  bool skip_prespray = false;
  bool burst_refresh = false;
  int prespray_times = 3;
  bool use_ga_reorder = true;
  // other
  MachineModules expected_module = MachineModules::NONE;
  float rotary_y_ratio = 1;
  float nozzle_voltage = NAN;
  float nozzle_pulse_width = NAN;
  int watt = 0;
};

class ToolpathExporterFcode : public QObject {
  Q_OBJECT

 public:
  ToolpathExporterFcode(const QJsonObject* param,
                        const QString* thumbnail) noexcept;

  std::string toString();
  void save(QDataStream* out);
  float getTimeCost();
  QJsonObject getMetadata();
  bool convertStack(const QList<LayerPtr>& layers, QProgressDialog* dialog = nullptr);

 Q_SIGNALS:
  void progressChanged(int value);

 public Q_SLOTS:
  void handleCancel();

 private:
  ToolpathProcessor proc;
  std::unique_ptr<BaseBitmapFactory> factory_;
  std::unique_ptr<BaseBitmapFactory> laser_filled_factory_;  // Use another factory to keep filled path data
  std::unique_ptr<LaserPathFactory> laser_path_factory_;
  QVector<std::shared_ptr<Workspace>> workspaces_ = {};
  QVector<ShapePtr> laser_bitmaps_;

  // Basic config
  Config config_;
  HardwareType hardware_ = HardwareType::Beambox;
  HardwareProfile hw_profile;
  SupportInfo support_info;
  std::shared_ptr<BaseMacros> macros;
  int magic_number_ = 0;
  bool is_v2_ = false;
  bool is_rotary_task_ = false;
  bool is_3d_task_ = false;
  bool has_job_origin_ = false;
  bool has_printing_task_ = false;
  QSizeF work_area_mm_;
  QTransform transform_base_ = QTransform::fromScale(1.0 / CANVAS_MM_RATIO, 1.0 / CANVAS_MM_RATIO);

  // Updated for each layer
  LayerPtr current_layer_;
  LayerPtr current_layer_2_;
  int layer_repeat_ = 1;
  float layer_speed_;       // mm/min
  float layer_path_speed_;  // mm/min
  float layer_backlash_ = 0;
  float layer_pwm_scale_ = 1;
  bool layer_is_high_quality_ = false;
  LayerModule layer_module_;
  bool is_laser_layer_ = false;
  bool is_printing_layer_ = false;
  bool is_uv_layer_ = false;
  QPointF layer_offset_;
  PrintingColor layer_color_;
  InwardRect layer_clip_;  // mm
  cv::Mat kernel_;

  QJsonArray post_config_;
  int current_layer_id_ = 0;
  LayerModule last_module_ = LayerModule::NONE;
  PrintingColor last_color_;
  QString last_sub_type_;

  // Updated during processing
  QSet<LayerModule> all_modules_;
  ConvertTarget convert_target_ = ConvertTarget::ALL;
  bool did_home_z_ = false;
  QTransform global_transform_;
  LayerModule prespray_module_ = LayerModule::NONE;
  // Task progress
  QProgressDialog* dialog_ = nullptr;
  bool cancelled_ = false;
  int total_layer_cnt_ = 1;
  int processed_layer_cnt_ = 0;
  int total_repeat_times_ = 1;
  int processed_repeat_times_ = 0;
  int element_cnt_[2] = {0, 0};  // 0 Path, 1 Filled Path + Bitmap
  int total_element_cnt_ = 0;
  double bitmap_progress_unit_ = 0;
  double current_progress_ = 0;  // progress within current repeat
  int progress_ = 0;

  void parseParam(const QJsonObject& paramPtr);
  void setTransform(QTransform transform = QTransform());
  InwardRect getClipRect(InwardRect current, QPointF offset, LayerModule module, bool rotary = false);
  void onProgressChanged(double value, bool absolute);
  // Layer task
  void convertLayer();
  void preprocessLaserLayer();
  void convertLaserLayer();
  void outputLayerPathFcode();
  void outputBitmapFcode();
  void convertPrintingLayer();
  // Sub task / block
  void outputPrintingTestFcode();
  void writePreviewImage();
  // Shape conversion
  bool convertShape(const ShapePtr& shape, bool from_group = false);
  bool convertGroup(const GroupShape* group);
  void convertBitmap(const BitmapShape* bmp);
  void convertPath(const PathShape* path);
  // Image processing functions
  void clearWhite(QImage* src, QRect dirty_area);
  void clearTransparent(QImage* src);
  void dilateBinaryBitmap(QImage* src);
  // Move related functions
  void homeZAxis();
  void backToHome();
};
