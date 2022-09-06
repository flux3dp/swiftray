#include "rt_status_update_executor.h"

#include <QDebug>
#include <QThread>

RTStatusUpdateExecutor::RTStatusUpdateExecutor(QPointer<MotionController> motion_controller, 
                                              QObject *parent)
  : Executor{parent}, motion_controller_{motion_controller}
{
  qInfo() << "RTStatusUpdateExecutor created";
  timer_ = new QTimer(this);
  hangning_detect_timer_ = new QTimer(this);
  hangning_detect_timer_->setSingleShot(true);
  connect(timer_, &QTimer::timeout, this, &RTStatusUpdateExecutor::exec);
  connect(hangning_detect_timer_, &QTimer::timeout, this, [=](){
    emit hanging();
  });
  connect(motion_controller_, &MotionController::realTimeStatusReceived,
          this, &RTStatusUpdateExecutor::onReportRcvd);
  connect(motion_controller_, &MotionController::disconnected, 
          this, &RTStatusUpdateExecutor::stop);
}

RTStatusUpdateExecutor::~RTStatusUpdateExecutor() {
  qInfo() << "RTStatusUpdateExecutor destructed";
  //timer_->deleteLater();
  //hangning_detect_timer_->deleteLater();
}

void RTStatusUpdateExecutor::start() {
  timer_->start(500);
  responded_ = true;
}

void RTStatusUpdateExecutor::exec() {
  //qInfo() << "RTStatusUpdateExecutor exec()";
  if (!responded_) {
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
  emit Executor::finished();
}
