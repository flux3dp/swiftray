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

QString Executor::stateToString(State state) {
  switch (state) {
    case State::kIdle:
      return tr("Ready");
    case State::kRunning:
      return tr("Running");
    case State::kPaused:
      return tr("Paused");
    case State::kCompleted:
      return tr("Finished");
    case State::kStopped:
      return tr("Stopped");
    default:
      return tr("Undefined State");
  }
}
