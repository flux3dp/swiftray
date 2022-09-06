#include "machine_setup_executor.h"

#include <QDebug>

MachineSetupExecutor::MachineSetupExecutor(QObject *parent)
  : Executor{parent}
{
  qInfo() << "MachineSetupExecutor created";
  timer_ = new QTimer(this);
  connect(timer_, &QTimer::timeout, this, &MachineSetupExecutor::exec);

  // TODO: non single shot
  timer_->setSingleShot(true);
}

void MachineSetupExecutor::attachMotionController(QPointer<MotionController> motion_controller) {
  if (!motion_controller_.isNull()) {
    // If already attached, detach first
    disconnect(motion_controller_, nullptr, this, nullptr);
    motion_controller_.clear();
    stop();
  }
  motion_controller_ = motion_controller;
  connect(motion_controller_, &MotionController::disconnected,
          this, &MachineSetupExecutor::stop);
}

void MachineSetupExecutor::start() {
  stop();
  // TODO: 
  qInfo() << "MachineSetupExecutor::start()";
  timer_->start(100);
}

void MachineSetupExecutor::exec() {
  stopped_ = false;
  qInfo() << "MachineSetupExecutor exec()";

  if (motion_controller_.isNull()) {
    stop();
    return;
  }
  // "\x18": reset first
  // "$#": gcode parameters
  // "$G": gcode parser state
  // "$$": settings
  // "$I": build info
  

  emit Executor::finished();
}

void MachineSetupExecutor::stop() {
  stopped_ = true;
  timer_->stop();
}
