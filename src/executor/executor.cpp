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
    Q_EMIT stateChanged(state_);
  }
}

int Executor::getStatusId() {
  switch (state_) {
    case State::kIdle:
      return 0;
    case State::kRunning:
      return 16;
    case State::kPaused:
      return 48;
    case State::kCompleted:
      return 64;
    case State::kStopped:
      return 48;
    default:
      return -1;
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
