#include "executor.h"
#include <debug/debug-timer.h>
#include <QDebug>
#include <QThread>
#include <thread>

Executor::Executor(QObject *parent)
  : QObject{parent}
{

}

Executor::~Executor() {
  qInfo() << this << "::deconstructor()";
  this->thread_enabled_ = false;
  if (this->exec_thread_.joinable()) {
    this->exec_thread_.join();
  }
}

void Executor::startThread() {
  qInfo() << this << "::startThread()";
  if (!this->exec_thread_.joinable()) {
    this->exec_thread_ = std::thread(&Executor::execThread, this);
  } else {
    qInfo() << this << "::exec_thread is already running";
  }
}

void Executor::exec() {
  qInfo() << this << "::exec() not implemented"; // This is needed to avoid crash in deconstructor calling exec_thread.join()
}

void Executor::execThread() {
  this->thread_enabled_ = true;
  while (this->thread_enabled_) {
    this->exec_loop_count++;
    if (this->exec_loop_count % 1000 == 1) {
      qInfo() << this << "::threadFunction() alive @" << getDebugTime();
    }
    if (this->exec_wait > 0) {
      QThread::msleep(this->exec_wait);
      this->exec_wait = 0;
    }
    this->exec();
  }
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

void Executor::attachMotionController(QPointer<MotionController> motion_controller) {
  if (!motion_controller_.isNull()) {
    // If already attached, detach first
    disconnect(motion_controller_, nullptr, this, nullptr);
    motion_controller_.clear();
    handleStopped();
    changeState(State::kIdle);
  }
  qInfo() << this << "::attachMotionController()";
  motion_controller_ = motion_controller;
  connect(motion_controller_, &MotionController::statusUpdate, this, &Executor::handleMotionControllerStatusUpdate);
  connect(motion_controller_, &MotionController::disconnected, this, &Executor::handleStopped);
  connect(motion_controller_, &MotionController::resetDetected, this, &Executor::handleReset);
}

void Executor::handleCmdFinish(int result_code) {
  // qInfo() << this << "::handleCmdFinish() is not implemented";
}

void Executor::handlePaused() {
  // qInfo() << this << "::handlePaused() is not implemented";
}

void Executor::handleResume() {
  // qInfo() << this << "::handleResume() is not implemented";
}

void Executor::handleReset() {
  // qInfo() << this << "::handleReset() is not implemented";
}

void Executor::handleMotionControllerStatusUpdate(MotionControllerState mc_state, qreal x_pos, qreal y_pos, qreal z_pos) {
  // qInfo() << this << "::handleMotionControllerStatusUpdate() not implemented";
}