#pragma once

#include "base-factory.h"
#include "factory-constants.h"

class PrinterBitmapFactory : public BaseBitmapFactory {
 protected:
  bool is_4c = false;
  int slice_top_padding = SLICE_TOP_PADDING;
  int slice_bot_padding = SLICE_BOT_PADDING;
  int slice_width = DEFAULT_SLICE_WIDTH;
  int max_slice_width = DEFAULT_SLICE_WIDTH;
  int slice_height = DEFAULT_SLICE_HEIGHT;
  int max_slice_height = DEFAULT_SLICE_HEIGHT;
  double backlash = 0;  // mm, actually not used in printer
  int halftone = 1;  // 1: FM, 2: AM
  HalftoneParams halftone_params = {};
  QRect prespray_bbox = QRect();
  QRect cartridge_test_bbox = QRect();
  double prespray_width = 8;   // mm
  double prespray_safe_x = 2;  // mm
  NozzleMode nozzle_mode_ = NozzleMode::RIGHT;
  QMap<NozzleMode, QPointF> nozzle_offsets = {};
  int interpolation = 1;  // for uv only
  QVector<int> color_curve = {};
  QVector<PrintingColor> colors;
  QImage bitmap;  // source image and data after processing
  QRectF bitmap_dirty_area = QRectF();  // px according to current layer dpi
  QImage val_table;

  static FactoryKwargs normalize(FactoryKwargs kwargs) {
    if (!kwargs.pixel_per_mm) {
      kwargs.pixel_per_mm = DPMM_300;
    }
    if (kwargs.interpolation) {
      kwargs.pixel_per_mm = kwargs.pixel_per_mm.value() * kwargs.interpolation;
    }
    return kwargs;
  }

  // Canvas
  void setup_clip_rect(const std::shared_ptr<Workspace>& workspace) override;

  // Helpers
  virtual QPointF pixel_to_actual_position(
      int x,
      int y,
      NozzleMode nozzle_mode = NozzleMode::UNDEFINED,
      bool apply_backlash = false);

  // Task
  void generate_image_for_task(double black_ratio);
  void prepare_table();
  BlockBoxes slice_image(QRect box, bool reverse_y, int multipass);
  void generate_solid_block(QRect box,
                            float speed = 12000,
                            bool reverse_x = false,
                            NozzleMode nozzle_mode = NozzleMode::UNDEFINED,
                            int multipass = 1);
  void preprocess_box_data(const QRect& box);
  PacketData create_image_packet_data(const SlicedBox& box,
                                      int padding_left,
                                      int padding_right,
                                      bool reverse_x);
  void write_payload(NozzleMode printer_packet_type, const QByteArray& payload);
  virtual void write_data_to_proc(
      const SlicedBox& box,
      const QByteArray& payload,
      int px_count,
      float speed,
      bool reverse_x = false,
      bool force_y = false,
      NozzleMode nozzle_mode = NozzleMode::UNDEFINED);

 public:
  PrinterBitmapFactory(const FactoryKwargs& kwargs,
                       int slice_width = DEFAULT_SLICE_WIDTH,
                       int slice_height = DEFAULT_SLICE_HEIGHT,
                       int slice_top_padding = SLICE_TOP_PADDING,
                       int slice_bot_padding = SLICE_BOT_PADDING) noexcept;
  virtual ~PrinterBitmapFactory() = default;

  // Note: set_slice_xxx doesn't work with interpolation
  void set_slice_width(int val) override;
  void set_slice_height(int val) override;
  void set_slice_top_padding(int val) override;
  void set_slice_bot_padding(int val) override;
  void set_nozzle_mode(int val) override;
  void set_nozzle_offset(NozzleMode nozzle,
                         const QPointF& nozzle_offset) override;
  void generate_task_code(GenerateTaskKwargs kwargs) override;
  void set_preparatory_task_bbox(QRectF bbox) override;
  void generate_prespray_task_code(float speed,
                                   bool reverse_x,
                                   NozzleMode nozzle_mode) override;
  void generate_cartridge_task_code(int multipass,
                                    float speed,
                                    bool reverse_x,
                                    NozzleMode nozzle_mode) override;
};
