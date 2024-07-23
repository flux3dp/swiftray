#include "grbl_cmd.h"

GrblPauseCmd::GrblPauseCmd()
{
}

OperationCmd::ExecStatus GrblPauseCmd::execute(QPointer<Executor> executor, QPointer<MotionController> motion_controller_) {
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


GrblResumeCmd::GrblResumeCmd()
{
}

OperationCmd::ExecStatus GrblResumeCmd::execute(QPointer<Executor> executor, QPointer<MotionController> motion_controller_) {
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

GrblResetCmd::GrblResetCmd()
{
}

OperationCmd::ExecStatus GrblResetCmd::execute(QPointer<Executor> executor, QPointer<MotionController> motion_controller_) {
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

GrblStatusReportCmd::GrblStatusReportCmd()
{
}

OperationCmd::ExecStatus GrblStatusReportCmd::execute(QPointer<Executor> executor, QPointer<MotionController> motion_controller_) {
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

GrblUnlockCmd::GrblUnlockCmd()
{
}

OperationCmd::ExecStatus GrblUnlockCmd::execute(QPointer<Executor> executor, QPointer<MotionController> motion_controller_) {
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

GrblHomeCmd::GrblHomeCmd()
{
}

OperationCmd::ExecStatus GrblHomeCmd::execute(QPointer<Executor> executor, QPointer<MotionController> motion_controller_) {
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

GrblBuildInfoCmd::GrblBuildInfoCmd()
{
}

OperationCmd::ExecStatus GrblBuildInfoCmd::execute(QPointer<Executor> executor, QPointer<MotionController> motion_controller_) {
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

GrblSettingsCmd::GrblSettingsCmd()
{
}

OperationCmd::ExecStatus GrblSettingsCmd::execute(QPointer<Executor> executor, QPointer<MotionController> motion_controller_) {
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

GrblGCodeParamCmd::GrblGCodeParamCmd()
{
}

OperationCmd::ExecStatus GrblGCodeParamCmd::execute(QPointer<Executor> executor, QPointer<MotionController> motion_controller_) {
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
