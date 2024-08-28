#include "machine_setup_executor.h"

#include "operation_cmd/grbl_cmd.h"
#include "operation_cmd/gcode_cmd.h"
#include <QDebug>

MachineSetupExecutor::MachineSetupExecutor(QObject *parent)
  : Executor{parent}
{
  qInfo() << "MachineSetupExecutor created";
  exec_timer_ = new QTimer(this);
  connect(exec_timer_, &QTimer::timeout, this, &MachineSetupExecutor::exec);
  connect(this, &MachineSetupExecutor::trigger, this, &MachineSetupExecutor::wakeUp);
}

void MachineSetupExecutor::start() {
  handleStopped();
  qInfo() << "MachineSetupExecutor::start()";
  pending_cmd_.clear();
  if (motion_controller_->type() != "BSL") {
    // Force a soft reset at the start of machine setup
    pending_cmd_.push_back(GrblCmdFactory::createGrblCmd(GrblCmdFactory::CmdType::kCtrlReset));
    pending_cmd_.push_back(GrblCmdFactory::createGrblCmd(GrblCmdFactory::CmdType::kSysBuildInfo));

    exec_timer_->start(200);
  } else {
    // pending_cmd_.push_back(std::make_shared<GCodeCmd>("M6"));
    // for(int i = 0; i < 30; i++) {
    //   pending_cmd_.push_back(std::make_shared<GCodeCmd>("Z10F600"));
    // }
    exec_timer_->start(10);
  }
}

void MachineSetupExecutor::exec() {
  qInfo() << "MachineSetupExecutor exec(), pending command" << pending_cmd_.size();

  if (motion_controller_.isNull()) {
    handleStopped();
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
      exec_timer_->stop();
      // finish
      Q_EMIT Executor::finished();
      return;
    } else {
      exec_timer_->stop(); // sleep until next trigger
      return;
    }
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
  exec_timer_->stop();
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
  Q_EMIT trigger();
  return ;
}

/**
 * @brief Wake up the work horse timer
 * 
 */
void MachineSetupExecutor::wakeUp() {
  if (!exec_timer_->isActive()) {
    exec_timer_->start();
  }
}
