#include "gcode_cmd.h"

GCodeCmd::GCodeCmd(QString gcode)
  : OperationCmd{}
{
  gcode_ = gcode;
}

/**
 * @brief Send GCode to motion controller
 * 
 * @return ExecStatus 
 */
OperationCmd::ExecStatus GCodeCmd::execute(QPointer<Executor> executor, QPointer<MotionController> motion_controller_) {
  if (motion_controller_.isNull()) {
    fail();
    return status_;
  }

  MotionController::CmdSendResult result = motion_controller_->sendCmdPacket(executor, gcode_);
  switch (result) {
    case MotionController::CmdSendResult::kBusy:
      return status_;
    case MotionController::CmdSendResult::kOk:
      status_ = ExecStatus::kProcessing;
      return status_;
    case MotionController::CmdSendResult::kInvalid:
      [[fallthrough]];
    case MotionController::CmdSendResult::kFail:
      [[fallthrough]];
    default:
      fail();
      return status_;
  }
}
