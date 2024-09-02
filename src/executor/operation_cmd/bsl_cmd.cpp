#include "bsl_cmd.h"
#include <debug/debug-timer.h>

BSLPauseCmd::BSLPauseCmd() : OperationCmd{}
{
}

OperationCmd::ExecStatus BSLPauseCmd::execute(QPointer<Executor> executor, QPointer<MotionController> motion_controller_) {
  if (motion_controller_.isNull()) {
    fail();
    return status_;
  }

  qInfo() << "BSLPauseCmd::execute()" << getDebugTime();
  BSLMotionController *bsl_controller = dynamic_cast<BSLMotionController*>(motion_controller_.data());
  BSLMotionController::CmdSendResult result = bsl_controller->pause();
  if (result == BSLMotionController::CmdSendResult::kOk) {
    succeed();
  } else {
    fail();
  }
  return status_;
}


BSLResumeCmd::BSLResumeCmd() : OperationCmd{}
{
}

OperationCmd::ExecStatus BSLResumeCmd::execute(QPointer<Executor> executor, QPointer<MotionController> motion_controller_) {
  if (motion_controller_.isNull()) {
    fail();
    return status_;
  }

  qInfo() << "BSLResumeCmd::execute()" << getDebugTime();
  BSLMotionController *bsl_controller = dynamic_cast<BSLMotionController*>(motion_controller_.data());
  BSLMotionController::CmdSendResult result = bsl_controller->resume();
  if (result == BSLMotionController::CmdSendResult::kOk) {
    succeed();
  } else {
    fail();
  }
  return status_;
}

BSLResetCmd::BSLResetCmd() : OperationCmd{}
{
}

OperationCmd::ExecStatus BSLResetCmd::execute(QPointer<Executor> executor, QPointer<MotionController> motion_controller_) {
  if (motion_controller_.isNull()) {
    fail();
    return status_;
  }

  qInfo() << "BSLResetCmd::execute()" << getDebugTime();
  BSLMotionController::CmdSendResult result = motion_controller_->stop();
  if (result == BSLMotionController::CmdSendResult::kOk) {
    succeed();
  } else {
    fail();
  }
  return status_;
}

BSLStatusReportCmd::BSLStatusReportCmd() : OperationCmd{}
{
}

OperationCmd::ExecStatus BSLStatusReportCmd::execute(QPointer<Executor> executor, QPointer<MotionController> motion_controller_) {
  if (motion_controller_.isNull()) {
    fail();
    return status_;
  }
  BSLMotionController::CmdSendResult result = motion_controller_->sendCmdPacket(executor, "?");
  if (result == BSLMotionController::CmdSendResult::kOk) {
    succeed();
  } else {
    fail();
  }
  return status_;
}

BSLUnlockCmd::BSLUnlockCmd() : OperationCmd{}
{
}

OperationCmd::ExecStatus BSLUnlockCmd::execute(QPointer<Executor> executor, QPointer<MotionController> motion_controller_) {
  if (motion_controller_.isNull()) {
    fail();
    return status_;
  }

  BSLMotionController::CmdSendResult result = motion_controller_->sendCmdPacket(executor, "$X\n");
  if (result == BSLMotionController::CmdSendResult::kOk) {
    status_ = ExecStatus::kProcessing;
  } else {
    fail();
  }
  return status_;
}

BSLHomeCmd::BSLHomeCmd() : OperationCmd{}
{
}

OperationCmd::ExecStatus BSLHomeCmd::execute(QPointer<Executor> executor, QPointer<MotionController> motion_controller_) {
  if (motion_controller_.isNull()) {
    fail();
    return status_;
  }

  BSLMotionController::CmdSendResult result = motion_controller_->sendCmdPacket(executor, "$H\n");
  if (result == BSLMotionController::CmdSendResult::kOk) {
    status_ = ExecStatus::kProcessing;
  } else {
    fail();
  }
  return status_;
}

BSLBuildInfoCmd::BSLBuildInfoCmd() : OperationCmd{}
{
}

OperationCmd::ExecStatus BSLBuildInfoCmd::execute(QPointer<Executor> executor, QPointer<MotionController> motion_controller_) {
  if (motion_controller_.isNull()) {
    fail();
    return status_;
  }

  BSLMotionController::CmdSendResult result = motion_controller_->sendCmdPacket(executor, "$I\n");
  if (result == BSLMotionController::CmdSendResult::kOk) { // sent
    status_ = OperationCmd::ExecStatus::kProcessing; // waiting for ack ("ok")
  } else {
    fail();
  }
  return status_;
}

BSLSettingsCmd::BSLSettingsCmd() : OperationCmd{}
{
}

OperationCmd::ExecStatus BSLSettingsCmd::execute(QPointer<Executor> executor, QPointer<MotionController> motion_controller_) {
  if (motion_controller_.isNull()) {
    fail();
    return status_;
  }

  BSLMotionController::CmdSendResult result = motion_controller_->sendCmdPacket(executor, "$$\n");
  if (result == BSLMotionController::CmdSendResult::kOk) {
    status_ = ExecStatus::kProcessing;
  } else {
    fail();
  }
  return status_;
}

BSLGCodeParamCmd::BSLGCodeParamCmd() : OperationCmd{}
{
}

OperationCmd::ExecStatus BSLGCodeParamCmd::execute(QPointer<Executor> executor, QPointer<MotionController> motion_controller_) {
  if (motion_controller_.isNull()) {
    fail();
    return status_;
  }

  BSLMotionController::CmdSendResult result = motion_controller_->sendCmdPacket(executor, "$G\n");
  if (result == BSLMotionController::CmdSendResult::kOk) {
    status_ = ExecStatus::kProcessing;
  } else {
    fail();
  }
  return status_;
}
