#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <QObject>
#include <QString>
#include <thread>
#include "operation_cmd/operation_cmd.h"
#include <periph/motion_controller/motion_controller.h>

class Executor : public QObject
{
  Q_OBJECT
public:
  enum class State {
    kIdle,
    kRunning,
    kPaused,
    kCompleted,
    kStopped
  };
  Q_ENUM(State)

  static QString stateToString(State state);
  int getStatusId();

  explicit Executor(QObject *parent = nullptr);
  virtual ~Executor();
  size_t inProgressCmdCnt();
  State getState() const;
  void startThread();
  virtual void handleCmdFinish(int result_code);
  virtual void attachMotionController(QPointer<MotionController> motion_controller);

Q_SIGNALS:
  void finished();
  void stateChanged(State new_state);
  //void error(QString err);

protected Q_SLOTS:
  virtual void exec();
  virtual void handlePaused();
  virtual void handleResume();
  virtual void handleStopped() = 0;
  virtual void handleMotionControllerStatusUpdate(MotionControllerState mc_state, qreal x_pos, qreal y_pos, qreal z_pos);

private: 
  void execThread();

protected:
  void changeState(State new_state);
  State state_ = State::kIdle;
  QList<std::shared_ptr<OperationCmd>> cmd_in_progress_;
  QPointer<MotionController> motion_controller_;
  std::thread exec_thread_;
  bool thread_enabled_;
  int exec_loop_count = 0;
  int exec_wait = 0;
};

#endif // EXECUTOR_H
