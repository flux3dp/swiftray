#ifndef SYNC_EXEC_COMMAND_H
#define SYNC_EXEC_COMMAND_H

#include "operation_cmd.h"
#include <executor/executor.h>

class SyncExecCmd : public OperationCmd {
public:
  explicit SyncExecCmd();

  ExecStatus execute(QPointer<Executor> executor, QPointer<MotionController> motion_controller) override;

private:
  QPointer<Executor> executor_;
};

#endif // SYNC_EXEC_COMMAND_H