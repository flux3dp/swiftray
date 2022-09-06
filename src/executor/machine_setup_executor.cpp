#include "machine_setup_executor.h"

#include <QDebug>

MachineSetupExecutor::MachineSetupExecutor(QPointer<MotionController> motion_controller, 
                                            QObject *parent)
  : Executor{parent}, motion_controller_{motion_controller}
{
  qInfo() << "MachineSetupExecutor created";
  timer_ = new QTimer(this);
  connect(timer_, &QTimer::timeout, this, &MachineSetupExecutor::exec);
  connect(motion_controller_, &MotionController::disconnected, 
          this, &MachineSetupExecutor::stop);

  // TODO: non single shot
  timer_->setSingleShot(true);
}

void MachineSetupExecutor::start() {
  // TODO: 
  qInfo() << "MachineSetupExecutor::start()";
  timer_->start(100);
}

void MachineSetupExecutor::exec() {
  stopped_ = false;
  qInfo() << "MachineSetupExecutor exec()";

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
  emit Executor::finished();
}
