#include "motion_controller.h"

#include <QDebug>

MotionController::MotionController(QObject *parent)
  : QObject{parent}
{

}

void MotionController::attachPort(SerialPort *port) {
  qInfo() << "MotionController::attachPort()";
  port_ = port;
  connect(port_, &SerialPort::disconnected, this, &MotionController::portDisconnected);
}

void MotionController::portDisconnected() {

  qInfo() << "MotionController::portDisconnected()";
  emit disconnected();
}