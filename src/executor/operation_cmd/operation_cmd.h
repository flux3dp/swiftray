#ifndef OPERATION_COMMAND_H
#define OPERATION_COMMAND_H

#include <QPointer>

class Executor;

class OperationCmd
{
public:
  enum ExecStatus {
    kIdle,
    kProcessing,
    kOk,
    kError,
  };

  explicit OperationCmd();
  ExecStatus getStatus();

  virtual ExecStatus execute(QPointer<Executor> executor) = 0;
  void succeed();
  void fail();

protected:
  ExecStatus status_ = ExecStatus::kIdle;
};

#endif // OPERATION_COMMAND_H
