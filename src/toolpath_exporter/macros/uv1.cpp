#include "uv1.h"
#include "prespray.h"
#include "toolpath_exporter/toolpath-exporter-constants.h"

UV1Macros::UV1Macros(ToolpathProcessor* proc, float travel_speed)
    : BaseMacros(proc, travel_speed) {
  implemented_funcs = {MacroFunc::move_to_refresh_position,
                       MacroFunc::test_cartridge};
}

void UV1Macros::prespray(float travel_speed,
                         float task_speed,
                         bool should_enter_printer_mode) {
  HardwareProfile hw_profile = HW_PROFILE[HardwareType::UV];
  float start_x = pre_spray_x;
  proc->moveto(NamedArgs().rz(0).rf(z_feedrate).set_is_travel());
  proc->sync_grbl_motion(0);
  proc->sleep(0.1);
  PresprayParams prespray_params;
  prespray_params.work_area_mm = QSizeF(hw_profile.width, hw_profile.length);
  prespray_params.module = LayerModule::PRINTER_4C;
  prespray_params.prespray = QRectF(start_x, pre_spray_y, 20, 12.7);
  prespray_params.travel_speed = travel_speed;
  prespray_params.task_speed = task_speed;
  prespray_params.repeat = 6;
  prespray_params.reverse_first = true;
  prespray_params.should_enter_printer_mode = should_enter_printer_mode;
  prespray_params.nozzle_mode = NozzleMode::BOTH;
  prespray_params.reverse_4c = hw_profile.reverse_4c;
  generate_prespray_code(*proc, prespray_params);
  proc->moveto(NamedArgs().rx(start_x + 24).rf(travel_speed).set_is_travel());
}

void UV1Macros::back_to_home() {
  proc->moveto(NamedArgs().rz(0).rf(z_feedrate).set_is_travel());
  proc->moveto(NamedArgs().rx(5).ry(5).rf(travel_speed).set_is_travel());
  proc->sync_grbl_motion(0);
  proc->grbl_system_cmd(0);
  proc->sync_motion_type2(185, 0);
  proc->sync_grbl_motion(0);
}

QPointF UV1Macros::move_to_refresh_position() {
  proc->set_is_main_task(false);
  proc->sync_grbl_motion(0);
  proc->sleep(0.1);
  float x = pre_spray_x;
  float y = pre_spray_y;
  proc->moveto(NamedArgs().rx(x).ry(y).rs(0).set_force_y().set_is_travel());
  return QPointF(x, y);
}

void UV1Macros::test_cartridge(int prespray_times) {
  prespray();
  back_to_home();
}
