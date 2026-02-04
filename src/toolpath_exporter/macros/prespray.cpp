#include "prespray.h"
#include "toolpath_exporter/factories/printer-4c.h"
#include "toolpath_exporter/factories/printer.h"
#include "toolpath_exporter/toolpath-utils.h"

void generate_prespray_code(ToolpathProcessor& proc,
                            const PresprayParams& params) {
  bool is_4c = params.module == LayerModule::PRINTER_4C;
  QPointF offset = params.module_offsets.value(params.module, QPointF(0, 0));
  if (params.has_job_origin) {
    offset += params.job_origin;
  }
  FactoryKwargs kwargs = {
      .proc = &proc,
      .clip_rect_mm = params.clip_rect_mm,
      .work_area_mm = params.work_area_mm,
      .offset = offset,
      .workspaces = params.workspaces,
  };
  std::unique_ptr<BaseBitmapFactory> factory =
      is_4c ? std::make_unique<PrinterBitmapFactory4C>(kwargs)
            : std::make_unique<PrinterBitmapFactory>(kwargs);
  if (is_4c && params.reverse_4c) {
    factory->set_reversed(true);
  }
  factory->set_preparatory_task_bbox(params.prespray);
  float move_x = params.prespray.x() - offset.x();
  float move_y = params.prespray.y() - offset.y();
  if (params.reverse_first) {
    move_x += params.prespray.width();
  }
  float orig_travel_speed = proc.get_travel_speed();
  proc.set_travel_speed(params.travel_speed);
  if (params.is_rotary_task && params.rotary_z_motion) {
    proc.moveto({.x = move_x, .s = 0, .force_y = true, .is_travel = true});
    proc.moveto({.y = move_y, .s = 0, .force_y = true, .is_travel = true});
  } else {
    proc.moveto({.x = move_x, .y = move_y, .s = 0, .force_y = true, .is_travel = true});
  }
  if (params.is_rotary_task && params.rotary_z_motion) {
    // rotary prespray height
    proc.moveto({.z = 35});
  }
  if (params.should_enter_printer_mode) {
    proc.enter_printer_mode();
  }
  // Prespray before test
  if (!is_4c) {
    // Fix saturation to 9 when prespray
    QByteArray payload = generate_nozzle_setting_payload(9);
    proc.write_printer_packet(17, payload);
  }
  for (int i = 0; i < params.repeat; i++) {
    bool reverse_x = params.reverse_first ? (i % 2 == 0) : (i % 2 == 1);
    NozzleMode nozzle = params.nozzle_mode;
    if (nozzle == NozzleMode::BOTH) {
      nozzle = (i % 2 == 0) ? NozzleMode::LEFT : NozzleMode::RIGHT;
    }
    factory->generate_prespray_task_code(params.task_speed, reverse_x, nozzle);
  }
  proc.wait_printer_mode_sync();
  // Printing test
  if (params.do_test) {
    if (!is_4c) {
      QByteArray payload = generate_nozzle_setting_payload(1);
      proc.write_printer_packet(17, payload);
    }
    for (int i = 0; i < params.repeat; i++) {
      bool reverse_x = params.reverse_first ? (i % 2 == 0) : (i % 2 == 1);
      factory->generate_cartridge_task_code(3, params.task_speed, reverse_x);
    }
    proc.wait_printer_mode_sync();
  }
  if (params.should_enter_printer_mode) {
    proc.exit_printer_mode();
  }
  proc.moveto({.s = 0});
  if (params.is_rotary_task && params.rotary_z_motion) {
    proc.moveto({.z = 1});
    proc.moveto({.y = 0, .force_y = true, .is_travel = true});
  }
  proc.moveto({.f = orig_travel_speed});
  proc.set_travel_speed(orig_travel_speed);
}
