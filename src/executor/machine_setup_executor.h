#ifndef MACHINESETUPEXECUTOR_H
#define MACHINESETUPEXECUTOR_H

#include <QObject>
#include "executor.h"
#include <motion_controller/motion_controller.h>
#include <QPointer>
#include <QTimer>

class MachineSetupExecutor : public Executor
{
public:
  explicit MachineSetupExecutor(QObject *parent = nullptr);

  void attachMotionController(QPointer<MotionController> motion_controller);

public slots:
  void start() override;
  void exec() override;
  void stop() override;

private:
  QPointer<MotionController> motion_controller_;
  QTimer *timer_;
  bool stopped_ = false;
};

#endif // MACHINESETUPEXECUTOR_H
