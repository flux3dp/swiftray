#pragma once

#include "toolpath_exporter/generators/fcode-generator.h"

enum MacroFunc {
  set_ref_position,
  set_should_retract_table,
  go_to_standby_pos,
  extend_table,
  remove_printer_lid,
  put_back_printer_lid,
  move_to_refresh_position,
  post_refresh_motion,
  test_cartridge,
  reset_table
};

class BaseMacros {
 protected:
  ToolpathProcessor* proc;
  float travel_speed = 7500;

 public:
  QSet<MacroFunc> implemented_funcs;

  BaseMacros(ToolpathProcessor* proc, float travel_speed = 7500)
      : proc(proc), travel_speed(travel_speed) {}
  virtual ~BaseMacros() = default;

  virtual void set_ref_position(float x_left = NAN,
                                float max_y = NAN,
                                float y_lid = NAN,
                                float z_magnet = NAN,
                                float z_table = NAN,
                                float z_brush = NAN) {}
  virtual void set_should_retract_table(bool retract) {}
  virtual void go_to_standby_pos(float feedrate = 6000,
                                 bool home_first = true,
                                 bool reset_table = true) {}
  virtual void extend_table(float z = 6.5, float travel_speed = 6000) {}
  virtual void remove_printer_lid() {}
  virtual void put_back_printer_lid() {}
  virtual QPointF move_to_refresh_position() { return QPointF(); }
  virtual void post_refresh_motion() {}
  virtual void test_cartridge(int prespray_times = 3) {}
  virtual void reset_table(float feedrate = 4800,
                           bool should_avoid_magnet = true,
                           bool is_y_first = false) {}
  virtual void post_table_motion() {}
};

// Note: implemented_funcs needs to be updated correctly in derived classes
inline bool hasattr(std::shared_ptr<BaseMacros> macros, MacroFunc func) {
  if (!macros) {
    return false;
  }
  return macros->implemented_funcs.contains(func);
}
