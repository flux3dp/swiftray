#include "motion_controller.h"

#include <QDebug>

MotionController::MotionController(QObject *parent)
  : QObject{parent}
{

}

void MotionController::attachPort(SerialPort *port) {
  qInfo() << "MotionController::attachPort()";
  port_ = port;
  connect(port_, &SerialPort::lineReceived, this, &MotionController::respReceived);
  connect(port_, &SerialPort::disconnected, this, &MotionController::disconnected);
}

MotionControllerState MotionController::getState() const {
  std::scoped_lock<std::mutex> lk(state_mutex_);
  return state_;
};

void MotionController::setState(MotionControllerState new_state) {
  {
    std::scoped_lock<std::mutex> lk(state_mutex_);
    state_ = new_state;
  }
}

std::tuple<qreal, qreal, qreal> MotionController::getPos() const {
  return {x_pos_, y_pos_, z_pos_};
}


void MotionController::enqueueCmdExecutor(QPointer<Executor> executor) {
  cmd_executor_queue_.push_back(executor);
  qInfo() << "enqueueCmdExecutor(), cnt in queue: " <<cmd_executor_queue_.size();
}

void MotionController::dequeueCmdExecutor() {
  cmd_executor_queue_.pop_front();
  qInfo() << "dequeueCmdExecutor(), cnt in queue: " <<cmd_executor_queue_.size();
}
