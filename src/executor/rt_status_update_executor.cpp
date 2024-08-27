#include "rt_status_update_executor.h"

#include <QDebug>
#include <QThread>
#include "operation_cmd/gcode_cmd.h"

RTStatusUpdateExecutor::RTStatusUpdateExecutor(QObject *parent)
  : Executor{parent}
{
  qInfo() << "RTStatusUpdateExecutor created";
  exec_timer_ = new QTimer(this);
  hangning_detect_timer_ = new QTimer(this);
  hangning_detect_timer_->setSingleShot(true);
  connect(exec_timer_, &QTimer::timeout, this, &RTStatusUpdateExecutor::exec);
  connect(hangning_detect_timer_, &QTimer::timeout, this, &RTStatusUpdateExecutor::hanging);
}

void RTStatusUpdateExecutor::attachMotionController(
    QPointer<MotionController> motion_controller) {
  if (!motion_controller_.isNull()) {
    // If already attached, detach first
    disconnect(motion_controller_, nullptr, this, nullptr);
    motion_controller_.clear();
    handleStopped();
    changeState(State::kIdle);
  }
  motion_controller_ = motion_controller;
  connect(motion_controller_, &MotionController::realTimeStatusUpdated,
          this, &RTStatusUpdateExecutor::onReportRcvd);
  connect(motion_controller_, &MotionController::disconnected,
          this, &RTStatusUpdateExecutor::handleStopped);
}

/**
 * @brief Re-initialize state and start
 * 
 */
void RTStatusUpdateExecutor::start() {
  handleStopped();
  changeState(State::kRunning);
  exec_timer_->start(1500);
}

void RTStatusUpdateExecutor::exec() {
  //qInfo() << "RTStatusUpdateExecutor exec()";
  if (motion_controller_.isNull()) {
    handleStopped();
    return;
  }
  // NOTE: Keep sending even when hanging
  // TODO:BSL Move query command to motion controller
  GCodeCmd cmd("?");
  cmd.execute(this, motion_controller_);
  
  if (!hanging_) {
    hanging_ = true;
    hangning_detect_timer_->start(6000);
  }
}

void RTStatusUpdateExecutor::handlePaused() {
  if (state_ == State::kRunning) {
    changeState(State::kPaused);
    exec_timer_->stop();
  }
}

void RTStatusUpdateExecutor::handleResume() {
  if (state_ == State::kPaused) {
    changeState(State::kRunning);
    exec_timer_->start();
  }
}

void RTStatusUpdateExecutor::onReportRcvd() {
  hangning_detect_timer_->stop();
  hanging_ = false;
}

void RTStatusUpdateExecutor::handleStopped() {
  if (state_ == State::kRunning || state_ == State::kPaused) {
    qInfo() << "RTStatusUpdateExecutor::stop()";
    changeState(State::kStopped);
    exec_timer_->stop();
    hangning_detect_timer_->stop();
  }
}

void RTStatusUpdateExecutor::handleCmdFinish(int result_code) {
  // Do nothing
  return;
}
