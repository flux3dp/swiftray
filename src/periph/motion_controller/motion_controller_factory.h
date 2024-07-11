#ifndef MOTIONCONTROLLERFACTORY_H
#define MOTIONCONTROLLERFACTORY_H

#include "motion_controller.h"
#include "grbl_motion_controller.h"
#include "bsl_motion_controller.h"
#include <settings/machine-settings.h>

class MotionControllerFactory
{
public:

  static MotionController* createMotionController(
      MachineSettings::MachineParam machine_settings, QObject *parent = nullptr) {
    switch (machine_settings.board_type) {
      case MachineSettings::MachineParam::BoardType::GRBL_2020:
        qInfo() << "Creating Grbl MotionController...";
        return new GrblMotionController{parent};
      case MachineSettings::MachineParam::BoardType::BSL_2024:
        qInfo() << "Creating BSL MotionController...";
        return new BSLMotionController{parent};
      // TODO:
      case MachineSettings::MachineParam::BoardType::FLUX_2020:
      case MachineSettings::MachineParam::BoardType::M2NANO_7:
      case MachineSettings::MachineParam::BoardType::RUIDA_2020:
      default:
        qInfo() << "Creating Unsupported MotionController...";
        Q_ASSERT_X(false, "MotionControllerFactory", "Support for this machine hasn't benn implemented yet!");
        break;
    }
    return new GrblMotionController{parent};
  }
};

#endif // MOTIONCONTROLLERFACTORY_H
