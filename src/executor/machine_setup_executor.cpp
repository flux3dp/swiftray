#include "machine_setup_executor.h"

#include <QDebug>

MachineSetupExecutor::MachineSetupExecutor(QPointer<MotionController> motion_controller, 
                                            QObject *parent)
  : Executor{parent}, motion_controller_{motion_controller}
{
  qInfo() << "MachineSetupExecutor created";
}

void MachineSetupExecutor::exec() {
  qInfo() << "MachineSetupExecutor exec()";

  emit Executor::finished();
}
