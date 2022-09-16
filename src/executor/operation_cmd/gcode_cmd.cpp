#include "gcode_cmd.h"

GCodeCmd::GCodeCmd()
  : OperationCmd{}
{

}

void GCodeCmd::setMotionController(QPointer<MotionController> motion_controller) {
  motion_controller_ = motion_controller;
}

void GCodeCmd::setGCode(QString gcode) {
  gcode_ = gcode;
}

/**
 * @brief Send GCode to motion controller
 * 
 * @return ExecStatus 
 */
OperationCmd::ExecStatus GCodeCmd::execute(QPointer<Executor> executor) {
  if (motion_controller_.isNull()) {
    fail();
    return status_;
  }
  if (false == motion_controller_->sendCmdPacket(executor, gcode_)) {
    return status_; // Remain idle
  }
  status_ = ExecStatus::kProcessing;
  return status_;
}
