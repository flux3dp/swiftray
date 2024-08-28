#include "console_executor.h"
#include <debug/debug-timer.h>
#include <QDebug>
#include <thread>
#include <mutex>
#include <condition_variable>

ConsoleExecutor::ConsoleExecutor(QObject *parent)
  : Executor{parent}, running_(false)
{
  qInfo() << "ConsoleExecutor created";
}

ConsoleExecutor::~ConsoleExecutor()
{
  handleStopped();
}

void ConsoleExecutor::start() {
  handleStopped();

  qInfo() << "ConsoleExecutor::start()";
  if (!exec_thread_.joinable()) {
    exec_thread_ = std::thread(&ConsoleExecutor::threadFunction, this);
  }
}

void ConsoleExecutor::threadFunction() {
  running_ = true;
  while (running_) {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return !pending_cmd_.isEmpty() || !running_; });

    if (!running_) break;

    exec();
  }
}

void ConsoleExecutor::exec() {
  qInfo() << "ConsoleExecutor::exec()" << getDebugTime();

  if (pending_cmd_.isEmpty()) {
    return;
  }

  if (motion_controller_.isNull()) {
    handleStopped();
    return;
  }

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
  running_ = false;
  cv_.notify_one();
  if (exec_thread_.joinable()) {
    exec_thread_.join();
  }
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
  {
    std::lock_guard<std::mutex> lock(mutex_);
    pending_cmd_.push_back(cmd);
  }
  if (!running_) {
    start();
  }
  cv_.notify_one();
}