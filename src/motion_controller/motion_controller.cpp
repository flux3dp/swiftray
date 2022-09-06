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
  std::lock_guard<std::mutex> lk(state_mutex_);
  return state_;
};
