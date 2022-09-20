#include "executor.h"

Executor::Executor(QObject *parent)
  : QObject{parent}
{

}

size_t Executor::inProgressCmdCnt() {
  return cmd_in_progress_.size();
}

void Executor::changeState(State new_state) {
  if (state_ != new_state) {
    state_ = new_state;
    emit stateChanged(state_);
  }
}

Executor::State Executor::getState() const {
  return state_;
}