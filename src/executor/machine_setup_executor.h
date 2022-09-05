#ifndef MACHINESETUPEXECUTOR_H
#define MACHINESETUPEXECUTOR_H

#include <QObject>
#include "executor.h"
#include <motion_controller/motion_controller.h>
#include <QPointer>

class MachineSetupExecutor : public Executor
{
public:
  explicit MachineSetupExecutor(QPointer<MotionController> motion_controller, 
                                QObject *parent = nullptr);

public slots:
  void exec() override;

private:
  QPointer<MotionController> motion_controller_;
};

#endif // MACHINESETUPEXECUTOR_H
