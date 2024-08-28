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
public:
  explicit ConsoleExecutor(QObject *parent = nullptr);
  ~ConsoleExecutor();

  void handleCmdFinish(int result_code) override;

  void appendCmd(std::shared_ptr<OperationCmd> cmd);
  void start() override;

private Q_SLOTS:
  void exec() override;
  void handlePaused() override {};
  void handleResume() override {};
  void handleStopped() override;
  void handleMotionControllerStateUpdate(MotionControllerState mc_state, qreal x_pos, qreal y_pos, qreal z_pos) override {};

private:
  QList<std::shared_ptr<OperationCmd>> pending_cmd_;
  QList<std::shared_ptr<OperationCmd>> cmd_in_progress_;

  std::thread exec_thread_;
  std::mutex mutex_;
  std::condition_variable cv_;
  bool running_;

  void threadFunction();
};