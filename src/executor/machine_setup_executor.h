#ifndef MACHINESETUPEXECUTOR_H
#define MACHINESETUPEXECUTOR_H

#include <QObject>
#include "executor.h"
#include <motion_controller/motion_controller.h>

class MachineSetupExecutor : public Executor
{
public:
  explicit MachineSetupExecutor(MotionController *motion_controller, 
                                QObject *parent = nullptr);

  void start() override;

private:
  MotionController *motion_controller_;
};

#endif // MACHINESETUPEXECUTOR_H
