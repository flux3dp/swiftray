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

  void start() override;
  void handleCmdFinish(int result_code) override;

private Q_SLOTS:
  void exec() override;
  void wakeUp();   // wake up this executor
  void handlePaused() override {};
  void handleResume() override {};
  void handleStopped() override;
  void handleMotionControllerStateUpdate(MotionControllerState mc_state, qreal x_pos, qreal y_pos, qreal z_pos) override {};


Q_SIGNALS:
  void trigger();  // wake up this executor

private:
  QTimer *exec_timer_;
  QList<std::shared_ptr<OperationCmd>> pending_cmd_;
  QList<std::shared_ptr<OperationCmd>> cmd_in_progress_;
};