#ifndef MOTIONCONTROLLERFACTORY_H
#define MOTIONCONTROLLERFACTORY_H

#include "motion_controller.h"
#include "grbl_motion_controller.h"
#include <settings/machine-settings.h>

class MotionControllerFactory
{
public:

  static MotionController* createMotionController(
      MachineSettings::MachineSet machine_settings, QObject *parent = nullptr) {
    switch (machine_settings.board_type) {
      case MachineSettings::MachineSet::BoardType::GRBL_2020:
        return new GrblMotionController{parent};
      // TODO:
      case MachineSettings::MachineSet::BoardType::FLUX_2020:
      case MachineSettings::MachineSet::BoardType::M2NANO_7:
      case MachineSettings::MachineSet::BoardType::RUIDA_2020:
      default:
        Q_ASSERT_X(false, "MotionControllerFactory", "Support for this machine hasn't benn implemented yet!");
        break;
    }
    return new GrblMotionController{parent};
  }
};

#endif // MOTIONCONTROLLERFACTORY_H
