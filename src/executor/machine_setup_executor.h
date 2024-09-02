#pragma once

#include <QObject>
#include "executor.h"
#include <QPointer>
#include <QTimer>

class MachineSetupExecutor : public Executor
{
  Q_OBJECT
public:
  explicit MachineSetupExecutor(QObject *parent = nullptr);

  void handleCmdFinish(int result_code) override;
  void attachMotionController(QPointer<MotionController> motion_controller) override;

private Q_SLOTS:
  void exec() override;
  void handleStopped() override;


Q_SIGNALS:
  void trigger();  // wake up this executor

private:
  bool finished_ = false;
  QTimer *exec_timer_;
  QList<std::shared_ptr<OperationCmd>> pending_cmd_;
  QList<std::shared_ptr<OperationCmd>> cmd_in_progress_;
};