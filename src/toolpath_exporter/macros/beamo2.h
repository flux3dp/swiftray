#pragma once

#include "base-macros.h"

class Beamo2Macros : public BaseMacros {
 private:
  float x_left = 0;
  float y_max = 245;
  float y_lid = 244;
  float z_brush = NAN;
  float z_magnet = 6.5;
  float z_table = 3;
  float z_feedrate = 500;
  bool should_retract_table = true;

  float magnet_x() const;
  float lid_x() const;
  float safe_y() const;
  float brush_z() const;

  void prespray(float travel_speed = 7500,
                float task_speed = 900,
                bool should_enter_printer_mode = true);
  void reset_table(float feedrate = 4800,
                   bool should_avoid_magnet = true,
                   bool is_y_first = false);
  void set_printer_lid(bool is_lid_on = true,
                       float delta = 0.5,
                       float feedrate = 1000);
  void clean_printer(float feedrate = 3600, int repeat = 3);
  void post_table_motion();
  void repeat_test(int repeat = 5, bool do_prespray = true);

 public:
  Beamo2Macros(ToolpathProcessor* proc, float travel_speed = 7500);

  void set_ref_position(float x_left = NAN,
                        float max_y = NAN,
                        float y_lid = NAN,
                        float z_magnet = NAN,
                        float z_table = NAN,
                        float z_brush = NAN) override;
  void set_should_retract_table(bool retract) override;
  void go_to_standby_pos(float feedrate = 6000,
                         bool home_first = true,
                         bool reset_table = true) override;
  void extend_table(float z = 6.5, float travel_speed = 6000) override;
  void remove_printer_lid() override;
  void put_back_printer_lid() override;
  QPointF move_to_refresh_position() override;
  void post_refresh_motion() override;
  void test_cartridge() override;
};
