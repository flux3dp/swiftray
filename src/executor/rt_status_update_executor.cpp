#include "rt_status_update_executor.h"

#include <QDebug>
#include <QThread>

RTStatusUpdateExecutor::RTStatusUpdateExecutor(QObject *parent)
  : Executor{parent}
{
  qInfo() << "RTStatusUpdateExecutor created";
  timer_ = new QTimer(this);
  hangning_detect_timer_ = new QTimer(this);
  hangning_detect_timer_->setSingleShot(true);
  connect(timer_, &QTimer::timeout, this, &RTStatusUpdateExecutor::exec);
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

void RTStatusUpdateExecutor::start() {
  stop();
  timer_->start(500);
  responded_ = true;
}

void RTStatusUpdateExecutor::exec() {
  //qInfo() << "RTStatusUpdateExecutor exec()";
  if (!responded_) {
    return;
  }
  if (motion_controller_.isNull()) {
    stop();
    return;
  }
  motion_controller_->sendCtrlCmd(MotionControllerCtrlCmd::kStatusReport);
  responded_ = false;
  hangning_detect_timer_->start(8000);
}

void RTStatusUpdateExecutor::onReportRcvd() {
  responded_ = true;
  hangning_detect_timer_->stop();
}

void RTStatusUpdateExecutor::stop() {
  qInfo() << "RTStatusUpdateExecutor::stop()";
  timer_->stop();
  hangning_detect_timer_->stop();
}
