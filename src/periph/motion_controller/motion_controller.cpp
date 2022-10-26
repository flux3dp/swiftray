#include "motion_controller.h"

#include <QDebug>

MotionController::MotionController(QObject *parent)
  : QObject{parent}
{

}
#ifdef CUSTOM_SERIAL_PORT_LIB
void MotionController::attachPort(SerialPort *port) {
  qInfo() << "MotionController::attachPort()";
  port_ = port;
  connect(port_, &SerialPort::lineReceived, this, &MotionController::respReceived);
  connect(port_, &SerialPort::disconnected, this, &MotionController::disconnected);
}
#else
void MotionController::attachPort(QSerialPort *port) {
  qInfo() << "MotionController::attachPort()";
  port_ = port;
  port_->clear();
  unprocssed_response_.clear();

  // Handle response
  connect(port_, &QSerialPort::readyRead, this, [=]() {
    QByteArray new_data = port_->readAll();
    QByteArray resp_data = unprocssed_response_ + new_data;
    QString resp_str = QString::fromUtf8(resp_data);
    int processed_chars = 0;
    for (int i = 0; i < resp_str.length(); i++) {
      if (resp_str[i] == "\n") {
        respReceived(resp_str.right(resp_str.length() - processed_chars).left(i - processed_chars).trimmed());
        processed_chars = i + 1;
      }
    }
    unprocssed_response_ = resp_data.right(resp_data.length() - processed_chars);
  });

  // Handle disconnect
  connect(port_, &QSerialPort::errorOccurred, this, [=](QSerialPort::SerialPortError error) {
    if (error == QSerialPort::ResourceError) {
      detachPort();
    }
  });
}

void MotionController::detachPort() {
  if (port_ && port_->isOpen()) {
    port_->close();
    port_ = nullptr;
    Q_EMIT disconnected();
  }
}
#endif


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
