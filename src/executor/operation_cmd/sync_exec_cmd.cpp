#include "sync_exec_cmd.h"


SyncExecCmd::SyncExecCmd()
: OperationCmd{} {
}

/**
 * @brief Sync the commands in an executor
 *        Wait until all cmd in progress to be finished
 *
 * @return ExecStatus
 */
OperationCmd::ExecStatus SyncExecCmd::execute(QPointer<Executor> executor, QPointer<MotionController> motion_controller) {
  if (executor.isNull()) {
    succeed();
    return status_;
  }

  if (executor->inProgressCmdCnt() == 0) {
    succeed();
  }

  return status_;
}
