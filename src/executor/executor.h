#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <QObject>
#include <QString>
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
  size_t inProgressCmdCnt();
  State getState() const;
  virtual void start() = 0;
  virtual void handleCmdFinish(int result_code) = 0;
  void attachMotionController(QPointer<MotionController> motion_controller);

Q_SIGNALS:
  void finished();
  void stateChanged(State new_state);
  //void error(QString err);

protected Q_SLOTS:
  virtual void exec() = 0;
  virtual void handlePaused() = 0;
  virtual void handleResume() = 0;
  virtual void handleStopped() = 0;
  virtual void handleMotionControllerStateUpdate(MotionControllerState mc_state, qreal x_pos, qreal y_pos, qreal z_pos) = 0;

protected:
  void changeState(State new_state);
  State state_ = State::kIdle;
  QList<std::shared_ptr<OperationCmd>> cmd_in_progress_;
  QPointer<MotionController> motion_controller_;
};

#endif // EXECUTOR_H
