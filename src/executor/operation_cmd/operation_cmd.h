#ifndef OPERATION_COMMAND_H
#define OPERATION_COMMAND_H

#include <QPointer>

class Executor;

class OperationCmd
{
public:
  /*
  enum class Target {
    kNone,
    kMotionControl,
    //kAutofocusControl,
    //kCameraControl
  };
  */
  enum ExecStatus {
    kIdle,
    kProcessing,
    kOk,
    kError,
  };

  explicit OperationCmd();
  //void setTarget(Target);
  ExecStatus getStatus();

  virtual ExecStatus execute(QPointer<Executor> executor) = 0;
  void succeed();
  void fail();

protected:
  ExecStatus status_ = ExecStatus::kIdle;
  //Target target_ = Target::kNone;
};

#endif // OPERATION_COMMAND_H
