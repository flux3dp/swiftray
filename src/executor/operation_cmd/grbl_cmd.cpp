#include "grbl_cmd.h"

GrblPauseCmd::GrblPauseCmd(QPointer<MotionController> motion_controller)
 : motion_controller_(motion_controller)
{
}

OperationCmd::ExecStatus GrblPauseCmd::execute(QPointer<Executor> executor) {
  if (motion_controller_.isNull()) {
    fail();
    return status_;
  }

  MotionController::CmdSendResult result = motion_controller_->sendCmdPacket(executor, "!");
  if (result == MotionController::CmdSendResult::kOk) {
    succeed();
  } else {
    fail();
  }
  return status_;
}


GrblResumeCmd::GrblResumeCmd(QPointer<MotionController> motion_controller) 
 : motion_controller_(motion_controller)
{
}

OperationCmd::ExecStatus GrblResumeCmd::execute(QPointer<Executor> executor) {
  if (motion_controller_.isNull()) {
    fail();
    return status_;
  }

  MotionController::CmdSendResult result = motion_controller_->sendCmdPacket(executor, "~");
  if (result == MotionController::CmdSendResult::kOk) {
    succeed();
  } else {
    fail();
  }
  return status_;
}

GrblResetCmd::GrblResetCmd(QPointer<MotionController> motion_controller) 
 : motion_controller_(motion_controller)
{
}

OperationCmd::ExecStatus GrblResetCmd::execute(QPointer<Executor> executor) {
  if (motion_controller_.isNull()) {
    fail();
    return status_;
  }

  MotionController::CmdSendResult result = motion_controller_->sendCmdPacket(executor, "\x18");
  if (result == MotionController::CmdSendResult::kOk) {
    succeed();
  } else {
    fail();
  }
  return status_;
}

GrblStatusReportCmd::GrblStatusReportCmd(QPointer<MotionController> motion_controller) 
 : motion_controller_(motion_controller)
{
}

OperationCmd::ExecStatus GrblStatusReportCmd::execute(QPointer<Executor> executor) {
  if (motion_controller_.isNull()) {
    fail();
    return status_;
  }

  MotionController::CmdSendResult result = motion_controller_->sendCmdPacket(executor, "?");
  if (result == MotionController::CmdSendResult::kOk) {
    succeed();
  } else {
    fail();
  }
  return status_;
}

GrblUnlockCmd::GrblUnlockCmd(QPointer<MotionController> motion_controller) 
 : motion_controller_(motion_controller)
{
}

OperationCmd::ExecStatus GrblUnlockCmd::execute(QPointer<Executor> executor) {
  if (motion_controller_.isNull()) {
    fail();
    return status_;
  }

  MotionController::CmdSendResult result = motion_controller_->sendCmdPacket(executor, "$X\n");
  if (result == MotionController::CmdSendResult::kOk) {
    status_ = ExecStatus::kProcessing;
  } else {
    fail();
  }
  return status_;
}

GrblHomeCmd::GrblHomeCmd(QPointer<MotionController> motion_controller) 
 : motion_controller_(motion_controller)
{
}

OperationCmd::ExecStatus GrblHomeCmd::execute(QPointer<Executor> executor) {
  if (motion_controller_.isNull()) {
    fail();
    return status_;
  }

  MotionController::CmdSendResult result = motion_controller_->sendCmdPacket(executor, "$H\n");
  if (result == MotionController::CmdSendResult::kOk) {
    status_ = ExecStatus::kProcessing;
  } else {
    fail();
  }
  return status_;
}

GrblBuildInfoCmd::GrblBuildInfoCmd(QPointer<MotionController> motion_controller) 
 : motion_controller_(motion_controller)
{
}

OperationCmd::ExecStatus GrblBuildInfoCmd::execute(QPointer<Executor> executor) {
  if (motion_controller_.isNull()) {
    fail();
    return status_;
  }

  MotionController::CmdSendResult result = motion_controller_->sendCmdPacket(executor, "$I\n");
  if (result == MotionController::CmdSendResult::kOk) { // sent
    status_ = ExecStatus::kProcessing; // waiting for ack ("ok")
  } else {
    fail();
  }
  return status_;
}

GrblSettingsCmd::GrblSettingsCmd(QPointer<MotionController> motion_controller) 
 : motion_controller_(motion_controller)
{
}

OperationCmd::ExecStatus GrblSettingsCmd::execute(QPointer<Executor> executor) {
  if (motion_controller_.isNull()) {
    fail();
    return status_;
  }

  MotionController::CmdSendResult result = motion_controller_->sendCmdPacket(executor, "$$\n");
  if (result == MotionController::CmdSendResult::kOk) {
    status_ = ExecStatus::kProcessing;
  } else {
    fail();
  }
  return status_;
}

GrblGCodeParamCmd::GrblGCodeParamCmd(QPointer<MotionController> motion_controller) 
 : motion_controller_(motion_controller)
{
}

OperationCmd::ExecStatus GrblGCodeParamCmd::execute(QPointer<Executor> executor) {
  if (motion_controller_.isNull()) {
    fail();
    return status_;
  }

  MotionController::CmdSendResult result = motion_controller_->sendCmdPacket(executor, "$G\n");
  if (result == MotionController::CmdSendResult::kOk) {
    status_ = ExecStatus::kProcessing;
  } else {
    fail();
  }
  return status_;
}
