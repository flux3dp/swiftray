#include "rt_status_update_executor.h"

#include <QDebug>
#include <QThread>

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
    stop();
  }
  motion_controller_ = motion_controller;
  connect(motion_controller_, &MotionController::realTimeStatusReceived,
          this, &RTStatusUpdateExecutor::onReportRcvd);
  connect(motion_controller_, &MotionController::disconnected,
          this, &RTStatusUpdateExecutor::stop);
}

/**
 * @brief Re-initialize state and start
 * 
 */
void RTStatusUpdateExecutor::start() {
  stop();
  exec_timer_->start(300);
}

void RTStatusUpdateExecutor::exec() {
  //qInfo() << "RTStatusUpdateExecutor exec()";
  if (motion_controller_.isNull()) {
    stop();
    return;
  }
  // NOTE: Keep sending even when hanging
  motion_controller_->sendCtrlCmd(MotionControllerCtrlCmd::kStatusReport);
  if (!hanging_) {
    hanging_ = true;
    hangning_detect_timer_->start(6000);
  }
}

void RTStatusUpdateExecutor::pause() {
  exec_timer_->stop();
}

void RTStatusUpdateExecutor::resume() {
  exec_timer_->start();
}

void RTStatusUpdateExecutor::onReportRcvd() {
  hangning_detect_timer_->stop();
  hanging_ = false;
}

void RTStatusUpdateExecutor::stop() {
  qInfo() << "RTStatusUpdateExecutor::stop()";
  exec_timer_->stop();
  hangning_detect_timer_->stop();
}
