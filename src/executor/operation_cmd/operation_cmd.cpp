#include "operation_cmd.h"

OperationCmd::OperationCmd()
{

}

//void OperationCmd::setTarget(Target target) {
//  target_ = target;
//}

OperationCmd::ExecStatus OperationCmd::getStatus() {
  return status_;
}

void OperationCmd::succeed() {
  status_ = ExecStatus::kOk;
}

void OperationCmd::fail() {
  status_ = ExecStatus::kError;
}
