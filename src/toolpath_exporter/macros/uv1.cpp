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
  proc->moveto({.z = 0, .f = z_feedrate, .is_travel = true});
  proc->sync_grbl_motion(0);
  proc->sleep(0.1);
  generate_prespray_code(
      *proc, {
                 .work_area_mm = QSizeF(hw_profile.width, hw_profile.length),
                 .module = LayerModule::PRINTER_4C,
                 .prespray = QRectF(start_x, pre_spray_y, 20, 12.7),
                 .travel_speed = travel_speed,
                 .task_speed = task_speed,
                 .repeat = 6,
                 .reverse_first = true,
                 .should_enter_printer_mode = should_enter_printer_mode,
                 .nozzle_mode = NozzleMode::BOTH,
                 .reverse_4c = hw_profile.reverse_4c,
             });
  proc->moveto({.x = start_x + 24, .f = travel_speed, .is_travel = true});
}

void UV1Macros::back_to_home() {
  proc->moveto({.z = 0, .f = z_feedrate, .is_travel = true});
  proc->moveto({.x = 5, .y = 5, .f = travel_speed, .is_travel = true});
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
  proc->moveto({.x = x, .y = y, .s = 0, .force_y = true, .is_travel = true});
  return QPointF(x, y);
}

void UV1Macros::test_cartridge() {
  prespray();
  back_to_home();
}
