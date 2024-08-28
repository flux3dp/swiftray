#pragma once

#include <QObject>
#include "executor.h"
#include <periph/motion_controller/motion_controller.h>
#include <QPointer>
#include <QTimer>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>

class ConsoleExecutor : public Executor
{
  Q_OBJECT
public:
  explicit ConsoleExecutor(QObject *parent = nullptr);

  void handleCmdFinish(int result_code) override;
  void appendCmd(std::shared_ptr<OperationCmd> cmd);

private Q_SLOTS:
  void exec() override;
  void handleStopped() override;

private:
  QList<std::shared_ptr<OperationCmd>> pending_cmd_;
  QList<std::shared_ptr<OperationCmd>> cmd_in_progress_;

  std::mutex mutex_;
};