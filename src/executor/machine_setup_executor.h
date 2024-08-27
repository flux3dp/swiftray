#pragma once

#include <QObject>
#include "executor.h"
#include <periph/motion_controller/motion_controller.h>
#include <QPointer>
#include <QTimer>

class MachineSetupExecutor : public Executor
{
  Q_OBJECT
public:
  explicit MachineSetupExecutor(QObject *parent = nullptr);

  void attachMotionController(QPointer<MotionController> motion_controller);
  void start() override;
  void handleCmdFinish(int result_code) override;

private Q_SLOTS:
  void exec() override;
  void wakeUp();   // wake up this executor
  void handlePaused() override {};
  void handleResume() override {};
  void handleStopped() override;


Q_SIGNALS:
  void trigger();  // wake up this executor

private:
  QPointer<MotionController> motion_controller_;
  QTimer *exec_timer_;
  QList<std::shared_ptr<OperationCmd>> pending_cmd_;
  QList<std::shared_ptr<OperationCmd>> cmd_in_progress_;
};