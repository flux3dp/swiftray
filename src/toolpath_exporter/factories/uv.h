#pragma once

#include "printer.h"

class UVBitmapFactory : public PrinterBitmapFactory {
 private:
  UVType uv_type = UVType::WHITE_INK;
  int x_step = 1;  // px
  int printing_repeat = 1;
  bool uv_curing_after = false;
  int uv_light_strength = 25;
  int uv_curing_repeat = 1;

  static FactoryKwargs normalize(FactoryKwargs kwargs) {
    if (!kwargs.pixel_per_mm) {
      kwargs.pixel_per_mm = DPMM_96;
    }
    return kwargs;
  }

  // Task
  void write_uv_light_strength(int val);
  PacketData create_image_packet_data(const SlicedBox& box,
                                      int padding_left,
                                      int padding_right,
                                      int current_step,
                                      bool reverse_x);
  void write_payload(NozzleMode printer_packet_type, const QByteArray& payload);
  void write_data_to_proc(
      const SlicedBox& box,
      const QByteArray& payload,
      int px_count,
      float speed,
      bool reverse_x = false,
      bool force_y = false,
      NozzleMode nozzle_mode = NozzleMode::UNDEFINED) override;
  void uv_cure_rows(QVector<RowBoxes> rows, float speed);

 public:
  UVBitmapFactory(FactoryKwargs& kwargs) noexcept;

  void set_uv_type(UVType val) override;
  void set_uv_x_step(int val) override;
  void set_uv_light_strength(int val) override;
  void set_uv_curing_after(bool val) override;
  void set_printing_repeat(int val) override;
  void set_uv_curing_repeat(int val) override;
  void generate_task_code(GenerateTaskKwargs kwargs) override;
};
