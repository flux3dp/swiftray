#ifndef CONSOLEEXECUTOR_H
#define CONSOLEEXECUTOR_H

#include <QObject>
#include "executor.h"
#include <periph/motion_controller/motion_controller.h>
#include <QPointer>
#include <QTimer>
#include <memory>

class ConsoleExecutor : public Executor
{
public:
  explicit ConsoleExecutor(QObject *parent = nullptr);
  void handleCmdFinish(int result_code) override;

  void attachMotionController(QPointer<MotionController> motion_controller);
  void appendCmd(std::shared_ptr<OperationCmd> cmd);

public Q_SLOTS:
  void start() override;
  void exec() override;
  void pause() override;
  void resume() override;
  void stop() override;

private:
  QPointer<MotionController> motion_controller_;
  QTimer *exec_timer_;
  QList<std::shared_ptr<OperationCmd>> pending_cmd_;
  QList<std::shared_ptr<OperationCmd>> cmd_in_progress_;

};

#endif // CONSOLEEXECUTOR_H
