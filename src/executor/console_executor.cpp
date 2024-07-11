#include "console_executor.h"

#include <QDebug>

ConsoleExecutor::ConsoleExecutor(QObject *parent)
  : Executor{parent}
{
  qInfo() << "ConsoleExecutor created";
  exec_timer_ = new QTimer(this);
  connect(exec_timer_, &QTimer::timeout, this, &ConsoleExecutor::exec);
}

void ConsoleExecutor::attachMotionController(QPointer<MotionController> motion_controller) {
  if (!motion_controller_.isNull()) {
    // If already attached, detach first
    disconnect(motion_controller_, nullptr, this, nullptr);
    motion_controller_.clear();
    stop();
  }
  motion_controller_ = motion_controller;
  connect(motion_controller_, &MotionController::disconnected,
          this, &ConsoleExecutor::stop);
}

void ConsoleExecutor::start() {
  stop();

  qInfo() << "ConsoleExecutor::start()";
  exec_timer_->start(100);
}

void ConsoleExecutor::exec() {
  qInfo() << "ConsoleExecutor exec()";

  if (pending_cmd_.isEmpty()) {
    exec_timer_->stop();
    return;
  }

  if (motion_controller_.isNull()) {
    stop();
    return;
  }

  OperationCmd::ExecStatus exec_status = (pending_cmd_.first())->execute(this, motion_controller_);
  if (exec_status == OperationCmd::ExecStatus::kIdle) {
    // retry later
    exec_timer_->stop();
  } else if (exec_status == OperationCmd::ExecStatus::kError) {
    pending_cmd_.pop_front();
  } else if (exec_status == OperationCmd::ExecStatus::kOk) {
    pending_cmd_.pop_front();
  } else if (exec_status == OperationCmd::ExecStatus::kProcessing) {
    cmd_in_progress_.push_back(pending_cmd_.first());
    pending_cmd_.pop_front();
  }
  
}

void ConsoleExecutor::pause() {

}

void ConsoleExecutor::resume() {
  
}

void ConsoleExecutor::stop() {
  exec_timer_->stop();
  pending_cmd_.clear();
  cmd_in_progress_.clear();
}

void ConsoleExecutor::handleCmdFinish(int result_code) {
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

void ConsoleExecutor::appendCmd(std::shared_ptr<OperationCmd> cmd) {
  pending_cmd_.push_back(cmd);
  if (!exec_timer_->isActive()) {
    exec_timer_->start();
  }
}
