#pragma once

#include "printer.h"
#include "toolpath_exporter/toolpath-exporter-constants.h"

class PrinterBitmapFactory4C : public PrinterBitmapFactory {
 private:
  QMap<PrintingColor, double> am_angle_map = AM_ANGLE_MAP_4C;
  QMap<PrintingColor, QVector<int>> color_curves_map;
  QMap<PrintingColor, QPoint> color_offsets = {
      {PrintingColor::CYAN, QPoint(0, 0)},
      {PrintingColor::MAGENTA, QPoint(42, 0)},
      {PrintingColor::YELLOW, QPoint(84, 0)},
      {PrintingColor::BLACK, QPoint(126, 0)},
  };
  int refresh_interval = 0;  // time counts (second) to refresh ink
  bool burst_refresh = false;
  int burst_refresh_counts = 300;
  double refresh_x_mm = 0;
  std::shared_ptr<BaseMacros> macros;
  bool is_reversed = false;

  static FactoryKwargs normalize(FactoryKwargs kwargs) {
    if (!kwargs.pixel_per_mm) {
      kwargs.pixel_per_mm = DPMM_600;
    }
    return kwargs;
  }

  // Helpers
  QPointF pixel_to_actual_position(int x,
                                   int y,
                                   NozzleMode nozzle_mode,
                                   bool apply_backlash) override;
  void reverse_offset();
  float get_padded_x(double x,
                     bool is_start,
                     bool reverse_x = false,
                     double padding = 0);

  // Canvas
  void align_workspace_dimensions();

  // Task
  void generate_4_color_block(QRect box,
                              float speed = 12000,
                              bool reverse_x = false,
                              NozzleMode nozzle_mode = NozzleMode::UNDEFINED,
                              int multipass = 1);
  void refresh_ink(int repeat = 3, double block_width_mm = 5, double y = NAN);
  PacketData4C create_image_packet_data_4c(const SlicedBox& box,
                                           bool reverse_x = false,
                                           bool skip_empty = true);
  double write_data_to_proc_4c(const SlicedBox& box,
                               const QByteArray& payload,
                               float speed,
                               bool reverse_x = false,
                               bool force_y = false,
                               bool is_start = true,
                               bool is_end = true,
                               bool should_adjust_height = false,
                               NozzleMode nozzle_mode = NozzleMode::UNDEFINED,
                               double padding = 0);

 public:
  PrinterBitmapFactory4C(FactoryKwargs& kwargs) noexcept;

  void set_reversed(bool val) override;
  void set_am_angle_map(const QString& raw_val) override;
  void set_color_curves_map(const QString& raw_val) override;
  void set_macros(std::shared_ptr<BaseMacros> macros_ptr) override;
  void set_refresh_x_mm(double val) override;
  void set_refresh_interval(int val) override;
  void set_burst_refresh(bool val) override;
  void add_image_by_color(QImage& img, QRectF& bbox, QColor color) override;
  void generate_prespray_task_code(float speed,
                                   bool reverse_x,
                                   NozzleMode nozzle_mode) override;
  void generate_task_code(GenerateTaskKwargs kwargs) override;
};
