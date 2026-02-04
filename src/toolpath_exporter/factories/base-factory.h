#pragma once

#include "factory-types.h"
#include "workspace.h"
#include "toolpath_exporter/generators/fcode-generator.h"
#include "toolpath_exporter/macros/base-macros.h"
#include <QImage>
#include <QVector>

class BaseFactory {
 protected:
  ToolpathProcessor* proc;
  std::function<void(double, bool)> onProgressChanged;
  bool cancelled = false;

  QPointF offset;  // mm
  double pixel_per_mm;
  double pixel_per_mm_x;
  QTransform transform;  // px
  QRect clip_rect;       // px

 public:
  BaseFactory(const FactoryKwargs& kwargs) noexcept;
  void handleCancel();
  QTransform get_transform();
};

class BaseBitmapFactory : public BaseFactory {
 protected:
  QVector<std::shared_ptr<Workspace>>* workspaces;
  QVector<bool> is_valid;
  bool use_own_workspaces = false;
  int default_workspaces_index = 0;

  bool one_way = false;
  bool split_bbox = false;
  QSize work_area;
  QSizeF work_area_mm;
  double pixel_size;
  double pixel_size_x;

  virtual void setup_clip_rect(const std::shared_ptr<Workspace>& workspace);
  QPointF pixel_to_actual_position(int x, int y);
  int get_padding_pixels(double padding_dist);

 public:
  BaseBitmapFactory(const FactoryKwargs& kwargs) noexcept;
  virtual ~BaseBitmapFactory();

  // Customize attributes
  // Laser
  virtual void set_default_workspace(int val) {};
  virtual void set_pwm_engraving(bool val) {};
  // Printer
  virtual void set_slice_width(int val) {};
  virtual void set_slice_height(int val) {};
  virtual void set_slice_top_padding(int val) {};
  virtual void set_slice_bot_padding(int val) {};
  virtual void set_nozzle_mode(int val) {};
  virtual void set_nozzle_offset(NozzleMode nozzle,
                                 const QPointF& nozzle_offset) {};
  // 4C
  virtual void set_reversed(bool val) {};
  virtual void set_am_angle_map(const QString& raw_val) {};
  virtual void set_color_curves_map(const QString& raw_val) {};
  virtual void set_macros(std::shared_ptr<BaseMacros> macros_ptr) {};
  virtual void set_refresh_x_mm(double val) {};
  virtual void set_refresh_interval(int val) {};
  virtual void set_refresh_threshold(int val) {};
  // UV
  virtual void set_uv_type(UVType val) {};
  virtual void set_uv_x_step(int val) {};
  virtual void set_uv_light_strength(int val) {};
  virtual void set_uv_curing_after(bool val) {};
  virtual void set_printing_repeat(int val) {};
  virtual void set_uv_curing_repeat(int val) {};

  // Canvas
  bool is_workspace_valid(int index = -1);
  std::shared_ptr<Workspace> get_workspace(int index = -1,
                                           bool need_setup = false);
  virtual void add_filled_path(QPainterPath& path,
                               QRectF& bbox,
                               int index = -1) {}
  void add_image(QImage& img, QRectF& bbox, int index = -1);
  virtual void add_image_by_color(QImage& img, QRectF& bbox, QColor color) {};

  // Task
  virtual void generate_task_code(GenerateTaskKwargs kwargs) {};
  virtual void set_preparatory_task_bbox(QRectF bbox) {};
  virtual void generate_prespray_task_code(
      float speed = 12000,
      bool reverse_x = false,
      NozzleMode nozzle_mode = NozzleMode::UNDEFINED) {};
  virtual void generate_cartridge_task_code(
      int multipass = 3,
      float speed = 12000,
      bool reverse_x = false,
      NozzleMode nozzle_mode = NozzleMode::UNDEFINED) {};
};
