#include "machine_setup_executor.h"
#include "operation_cmd/grbl_cmd.h"
#include "operation_cmd/gcode_cmd.h"
#include <QDebug>

MachineSetupExecutor::MachineSetupExecutor(QObject *parent)
  : Executor{parent}
{
  qInfo() << "MachineSetupExecutor created";
}

void MachineSetupExecutor::exec() {
  if (this->finished_) {
    exec_wait = 1000;
    return;
  }
  qInfo() << "MachineSetupExecutor exec(), pending command" << pending_cmd_.size();

  if (motion_controller_.isNull()) {
    handleStopped();
    exec_wait = 1000;
    return;
  }

  // Setup procedure:
  //   "\x18": soft reset first
  //         get machine brand and version info
  //   "$#": gcode parameters
  //   "$G": gcode parser state
  //   "$$": settings
  //   "$I": build info
  //   "$X": unlock immediately
  if (pending_cmd_.isEmpty()) {
    if (cmd_in_progress_.isEmpty()) {
      if (!this->finished_) {
        Q_EMIT Executor::finished();
        this->finished_ = true;
      }
    }
    return; // Skip if no pending command
  }

  OperationCmd::ExecStatus exec_status = (pending_cmd_.first())->execute(this, motion_controller_);
  if (exec_status == OperationCmd::ExecStatus::kIdle) {
    // retry later
    return;
  } else if (exec_status == OperationCmd::ExecStatus::kProcessing) {
    cmd_in_progress_.push_back(pending_cmd_.first());
    pending_cmd_.pop_front();
  } else { // Finish immediately: ok or error
    pending_cmd_.pop_front();
  }
  
}

void MachineSetupExecutor::handleStopped() {
  pending_cmd_.clear();
  cmd_in_progress_.clear();
}

/**
 * @brief Called by motion controller
 * 
 * @param result_code 
 */
void MachineSetupExecutor::handleCmdFinish(int result_code) {
  if (!cmd_in_progress_.isEmpty()) {
    if (result_code == 0) {
      cmd_in_progress_.first()->succeed();
    } else {
      cmd_in_progress_.first()->fail();
    }
    cmd_in_progress_.pop_front();
  } 
  return ;
}

void MachineSetupExecutor::attachMotionController(QPointer<MotionController> motion_controller) {
  pending_cmd_.clear();
  cmd_in_progress_.clear();
  this->finished_ = false;
  Executor::attachMotionController(motion_controller);
  this->pending_cmd_.clear();
  // Append commands to be executed base on motion controller type
  if (motion_controller->type() != "BSL") {
    // Force a soft reset at the start of machine setup
    this->pending_cmd_.push_back(GrblCmdFactory::createGrblCmd(GrblCmdFactory::CmdType::kCtrlReset));
    this->pending_cmd_.push_back(GrblCmdFactory::createGrblCmd(GrblCmdFactory::CmdType::kSysBuildInfo));
    exec_wait = 100;
  } else {
    exec_wait = 0;
  }
}