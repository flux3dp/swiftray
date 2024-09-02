#include "console_executor.h"
#include <debug/debug-timer.h>
#include <QDebug>
#include <thread>
#include <mutex>
#include <condition_variable>

ConsoleExecutor::ConsoleExecutor(QObject *parent): Executor{parent}
{
  qInfo() << "ConsoleExecutor created";
}

void ConsoleExecutor::exec() {
  if (motion_controller_.isNull()) {
    handleStopped();
    this->exec_wait = 1000;
    return;
  }
  
  std::lock_guard<std::mutex> lock(mutex_);
  if (pending_cmd_.isEmpty()) {
    this->exec_wait = 100;
    return;
  }
  qInfo() << "ConsoleExecutor::exec()" << getDebugTime();

  OperationCmd::ExecStatus exec_status = (pending_cmd_.first())->execute(this, motion_controller_);
  if (exec_status == OperationCmd::ExecStatus::kIdle) {
    // retry later
  } else if (exec_status == OperationCmd::ExecStatus::kError) {
    pending_cmd_.pop_front();
  } else if (exec_status == OperationCmd::ExecStatus::kOk) {
    pending_cmd_.pop_front();
  } else if (exec_status == OperationCmd::ExecStatus::kProcessing) {
    cmd_in_progress_.push_back(pending_cmd_.first());
    pending_cmd_.pop_front();
  }
}

void ConsoleExecutor::handleStopped() {
  std::lock_guard<std::mutex> lock(mutex_);
  pending_cmd_.clear();
  cmd_in_progress_.clear();
}

void ConsoleExecutor::handleCmdFinish(int result_code) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!cmd_in_progress_.isEmpty()) {
    if (result_code == 0) {
      cmd_in_progress_.first()->succeed();
    } else {
      cmd_in_progress_.first()->fail();
    }
    cmd_in_progress_.pop_front();
  }
}

void ConsoleExecutor::appendCmd(std::shared_ptr<OperationCmd> cmd) {
  qInfo() << "ConsoleExecutor::appendCmd()" << getDebugTime();
  std::lock_guard<std::mutex> lock(mutex_);
  pending_cmd_.push_back(cmd);
}