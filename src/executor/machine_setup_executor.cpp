#include "machine_setup_executor.h"

#include <QDebug>

MachineSetupExecutor::MachineSetupExecutor(MotionController *motion_controller, 
                                            QObject *parent)
  : Executor{parent}, motion_controller_{motion_controller}
{
  qInfo() << "MachineSetupExecutor created";
}

void MachineSetupExecutor::start() {
  qInfo() << "MachineSetupExecutor start()";

  emit Executor::finished();
}
