#pragma once

#include "base-macros.h"

class UV1Macros : public BaseMacros {
 private:
  float z_feedrate = 500;
  float pre_spray_x = 0;
  float pre_spray_y = 150;

  void back_to_home();
  void prespray(float travel_speed = 7500,
                float task_speed = 900,
                bool should_enter_printer_mode = true);

 public:
  UV1Macros(ToolpathProcessor* proc, float travel_speed = 7500);

  QPointF move_to_refresh_position() override;
  void test_cartridge() override;
};
