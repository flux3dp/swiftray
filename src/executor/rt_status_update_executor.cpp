#include "rt_status_update_executor.h"

#include <QDebug>
#include <QThread>
#include "operation_cmd/gcode_cmd.h"

RTStatusUpdateExecutor::RTStatusUpdateExecutor(QObject *parent)
  : Executor{parent}
{
  qInfo() << this << "created";
  watchdog_timer_ = new QTimer(this);
  watchdog_timer_->setSingleShot(true);
  connect(watchdog_timer_, &QTimer::timeout, this, &RTStatusUpdateExecutor::timeout);
  connect(this, &RTStatusUpdateExecutor::startWatchdog, this, &RTStatusUpdateExecutor::onStartWatchdog, Qt::QueuedConnection);
  connect(this, &RTStatusUpdateExecutor::stopWatchdog, this, &RTStatusUpdateExecutor::onStopWatchdog, Qt::QueuedConnection);
}

void RTStatusUpdateExecutor::exec() {
  // qInfo() << "RTStatusUpdateExecutor::exec()";
  if (motion_controller_.isNull()) {
    exec_wait = 1000;
    return;
  }
  
  if (motion_controller_->type() != "BSL") {
    GCodeCmd cmd("?");
    cmd.execute(this, motion_controller_);
    Q_EMIT startWatchdog();
  }
  exec_wait = 1500;
}

void RTStatusUpdateExecutor::handleMotionControllerStatusUpdate(MotionControllerState mc_state, qreal x_pos, qreal y_pos, qreal z_pos) {
  qInfo() << "RTStatusUpdateExecutor::handleMotionControllerStatusUpdate()";
  Q_EMIT stopWatchdog();
}

void RTStatusUpdateExecutor::handleStopped() {
  if (state_ == State::kRunning || state_ == State::kPaused) {
    qInfo() << "RTStatusUpdateExecutor::stop()";
    changeState(State::kStopped);
    Q_EMIT stopWatchdog();
  }
}

void RTStatusUpdateExecutor::onStartWatchdog() {
  if (!watchdog_timer_->isActive()) {
    watchdog_timer_->start(6000);
  }
}

void RTStatusUpdateExecutor::onStopWatchdog() {
  watchdog_timer_->stop();
}