#include "beamo2.h"
#include "prespray.h"
#include "toolpath_exporter/toolpath-exporter-constants.h"

Beamo2Macros::Beamo2Macros(ToolpathProcessor* proc, float travel_speed)
    : BaseMacros(proc, travel_speed) {
  implemented_funcs = {MacroFunc::set_ref_position,
                       MacroFunc::set_should_retract_table,
                       MacroFunc::go_to_standby_pos,
                       MacroFunc::extend_table,
                       MacroFunc::remove_printer_lid,
                       MacroFunc::put_back_printer_lid,
                       MacroFunc::move_to_refresh_position,
                       MacroFunc::post_refresh_motion,
                       MacroFunc::test_cartridge};
}

void Beamo2Macros::set_ref_position(float x_left,
                                    float max_y,
                                    float y_lid,
                                    float z_magnet,
                                    float z_table,
                                    float z_brush) {
  if (!std::isnan(x_left)) {
    this->x_left = x_left;
  }
  if (!std::isnan(max_y)) {
    this->y_max = max_y;
  }
  if (!std::isnan(y_lid)) {
    this->y_lid = y_lid;
  }
  if (!std::isnan(z_table)) {
    this->z_table = z_table;
  }
  if (!std::isnan(z_magnet)) {
    this->z_magnet = z_magnet;
  }
  if (!std::isnan(z_brush)) {
    this->z_brush = z_brush;
  }
}

float Beamo2Macros::magnet_x() const {
  return x_left + 68;
}

float Beamo2Macros::lid_x() const {
  return x_left;
}

float Beamo2Macros::safe_y() const {
  return y_max - 55;
}

float Beamo2Macros::brush_z() const {
  if (!std::isnan(z_brush)) {
    return z_brush;
  }
  return round(qMax(z_table - 2.1, -1.5) * 10) / 10;
}

void Beamo2Macros::set_should_retract_table(bool retract) {
  should_retract_table = retract;
}

void Beamo2Macros::reset_table(float feedrate,
                               bool should_avoid_magnet,
                               bool is_y_first) {
  float z = z_magnet;
  if (is_y_first) {
    proc->moveto(NamedArgs().ry(safe_y()).rf(travel_speed).set_is_travel());
    proc->moveto(NamedArgs().rx(magnet_x()).rf(travel_speed).set_is_travel());
  } else {
    proc->moveto(NamedArgs().rx(magnet_x()).ry(safe_y()).rf(travel_speed).set_is_travel());
  }
  proc->sync_grbl_motion(0);
  proc->moveto(NamedArgs().rz(z).rf(z_feedrate).set_is_travel());
  proc->sync_grbl_motion(0);
  proc->moveto(NamedArgs().ry(y_max).rf(feedrate).set_is_travel());
  proc->sync_grbl_motion(0);
  if (should_avoid_magnet) {
    proc->moveto(NamedArgs().rx(magnet_x() + 50).rf(travel_speed).set_is_travel());
    proc->moveto(NamedArgs().rz(0).rf(z_feedrate).set_is_travel());
    proc->sync_grbl_motion(0);
  }
}

void Beamo2Macros::go_to_standby_pos(float feedrate,
                                     bool home_first,
                                     bool should_reset_table) {
  if (home_first) {
    // Move to home position first to avoid player position mismatch
    QPointF home_pos = HW_PROFILE[HardwareType::BM2].home_position;
    proc->moveto(NamedArgs()
                     .rx(home_pos.x())
                     .ry(home_pos.y())
                     .rf(travel_speed)
                     .set_is_travel());
    proc->sync_grbl_motion(0);
    proc->grbl_system_cmd(0);  // $H
  }
  proc->moveto(NamedArgs().rz(0).rf(z_feedrate).set_is_travel());
  proc->sync_grbl_motion(0);
  if (should_reset_table) {
    reset_table(feedrate, false);
  }
}

void Beamo2Macros::extend_table(float z, float travel_speed) {
  z = z_magnet;
  proc->moveto(NamedArgs().ry(y_max - 5).rf(travel_speed).set_is_travel());
  proc->moveto(NamedArgs().rx(magnet_x()).rf(travel_speed).set_is_travel());
  proc->sync_grbl_motion(0);
  proc->sleep(0.05);
  proc->moveto(NamedArgs().rz(z).rf(z_feedrate).set_is_travel());
  proc->sync_grbl_motion(0);
  proc->sleep(0.05);

  QVector<QPair<float, float>> y_moves = {// QPair<y, f>
                                          {y_max - 4, 3000},
                                          {y_max - 2, 1500},
                                          {y_max, 200}};
  for (const auto& move : y_moves) {
    proc->moveto(NamedArgs().ry(move.first).rf(move.second).set_is_travel());
    proc->sync_grbl_motion(0);
  }
  proc->sleep(0.5);
  proc->moveto(NamedArgs().ry(safe_y()).rf(4800).set_is_travel());
  proc->sleep(0.1);
}

void Beamo2Macros::set_printer_lid(bool is_lid_on,
                                   float delta,
                                   float feedrate) {
  float x = lid_x();
  float y = y_lid;
  float z = is_lid_on ? (z_table + 1.5) : (z_table + 0.3);
  proc->moveto(NamedArgs().rz(0).rf(z_feedrate).set_is_travel());  // could be z=-1
  proc->sync_grbl_motion(0);
  proc->sleep(0.3);
  proc->moveto(NamedArgs().rx(x).rf(travel_speed).set_is_travel());
  proc->moveto(NamedArgs().ry(y).rf(travel_speed).set_is_travel());
  proc->sync_grbl_motion(0);
  proc->sleep(0.2);
  proc->moveto(NamedArgs().rz(z).rf(z_feedrate).set_is_travel());
  proc->sync_grbl_motion(0);

  if (delta > 0) {
    QVector<QPair<float, float>> xy_positions = {// QPair<x, y>
                                                 {x + delta, y}, {x - delta, y},
                                                 {x, y},         {x, y + delta},
                                                 {x, y - delta}, {x, y}};

    for (const auto& pos : xy_positions) {
      if (pos.second > y_max) {
        continue;
      }
      proc->moveto(NamedArgs().rx(pos.first).ry(pos.second).rf(feedrate).set_is_travel());
      proc->sleep(0.1);
    }
  }

  if (!is_lid_on) {
    proc->moveto(NamedArgs().ry(safe_y()).rf(800).set_is_travel());
  } else {
    proc->moveto(NamedArgs().rx(x).rf(1000).set_is_travel());
    proc->sync_grbl_motion(0);
    proc->sleep(0.5);
  }

  proc->moveto(NamedArgs().rz(0).rf(is_lid_on ? 1000 : z_feedrate).set_is_travel());
  proc->sync_grbl_motion(0);
  if (is_lid_on) {
    proc->moveto(NamedArgs().ry(safe_y()).rf(travel_speed).set_is_travel());
  }
  proc->user_selection_cmd(1);
}

void Beamo2Macros::clean_printer(float feedrate, int repeat) {
  float start_x = x_left + 125;
  float end_x = start_x - 50;
  float y = y_max - 1;
  proc->moveto(NamedArgs().rx(start_x).ry(y).rf(travel_speed).set_is_travel());
  proc->sync_grbl_motion(0);
  proc->sleep(0.1);
  proc->moveto(NamedArgs().rz(brush_z()).rf(z_feedrate).set_is_travel());
  proc->sync_grbl_motion(0);
  proc->sleep(0.1);
  for (int i = 0; i < repeat - 1; i++) {
    proc->moveto(NamedArgs()
                     .ry((i % 2 == 1) ? (y + 3) : y)
                     .rf(feedrate)
                     .set_is_travel());
    proc->moveto(NamedArgs().rx(end_x).rf(feedrate).set_is_travel());
    proc->moveto(NamedArgs().rx(start_x).rf(feedrate).set_is_travel());
  }
  proc->moveto(NamedArgs().ry(y).rf(feedrate).set_is_travel());
  proc->moveto(NamedArgs().rx(end_x).rf(feedrate).set_is_travel());
  proc->sync_grbl_motion(0);
}

void Beamo2Macros::prespray(float travel_speed,
                            float task_speed,
                            bool should_enter_printer_mode) {
  float start_x = x_left + 40;
  proc->moveto(NamedArgs().rz(0).rf(z_feedrate).set_is_travel());
  proc->sync_grbl_motion(0);
  proc->sleep(0.1);
  HardwareProfile hw_profile = HW_PROFILE[HardwareType::BM2];
  PresprayParams prespray_params;
  prespray_params.work_area_mm = QSizeF(hw_profile.width, hw_profile.length);
  prespray_params.module = LayerModule::PRINTER_4C;
  prespray_params.prespray = QRectF(start_x, y_max - 1, 20, 12.7);
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

void Beamo2Macros::post_table_motion() {
  proc->moveto(NamedArgs().rz(0).rf(z_feedrate).set_is_travel());
  proc->moveto(NamedArgs().rx(5).ry(5).rf(travel_speed).set_is_travel());
  proc->sync_grbl_motion(0);
  proc->grbl_system_cmd(0);
  proc->sync_motion_type2(185, 0);
  proc->sync_grbl_motion(0);
}

void Beamo2Macros::remove_printer_lid() {
  go_to_standby_pos();
  extend_table();
  set_printer_lid(false);
}

void Beamo2Macros::put_back_printer_lid() {
  go_to_standby_pos(6000, true, should_retract_table);
  if (should_retract_table) {
    extend_table();
  }
  set_printer_lid(true);
  reset_table(4800, true, true);
  post_table_motion();
}

QPointF Beamo2Macros::move_to_refresh_position() {
  proc->set_is_main_task(false);
  proc->sync_motion_type2(179, 3);
  go_to_standby_pos(6000, false, should_retract_table);
  if (should_retract_table) {
    extend_table();
  }
  proc->moveto(NamedArgs().rz(0).rf(z_feedrate).set_is_travel());
  proc->sync_grbl_motion(0);
  proc->sleep(0.1);
  float x = x_left + 60;
  float y = y_max - 1;
  proc->moveto(NamedArgs().rx(x).ry(y).rs(0).set_force_y().set_is_travel());
  return QPointF(x, y);
}

void Beamo2Macros::post_refresh_motion() {
  if (should_retract_table) {
    reset_table();
  }
  proc->set_is_main_task(true);
}

void Beamo2Macros::test_cartridge() {
  prespray();
  if (should_retract_table) {
    reset_table();
  }
  post_table_motion();
}

/**
 * This function is used to debug the repeat functionality of the macros.
 */
void Beamo2Macros::repeat_test(int repeat, bool do_prespray) {
  go_to_standby_pos();

  for (int i = 0; i < repeat; i++) {
    extend_table();
    set_printer_lid(false);
    if (do_prespray) {
      prespray();
    }
    reset_table();
    extend_table();
    set_printer_lid(true);
    reset_table(4800, true, true);
  }

  post_table_motion();
}
