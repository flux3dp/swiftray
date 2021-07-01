#include <connection/serialport-thread.h>
#include <QIODevice>
#include <QMessageBox>
#include <QDebug>

void SerialPortThread::playGcode(QString port, int baudrate, QStringList gcode) {
  const QMutexLocker locker(&mutex_);
  port_ = port;
  baudrate_ = baudrate;
  gcode_ = gcode;
  current_line_ = 0;
  if (!isRunning()) {
    start();
  } else {
    cond_.wakeOne();
  }
}

void SerialPortThread::run() {
  mutex_.lock();
  QSerialPort serial;
  serial.close();
  serial.setPortName(port_);
  serial.setBaudRate(baudrate_);

  qInfo() << "Connecting" << port_ << baudrate_;
  auto open_result = serial.open(QIODevice::ReadWrite);
  if (!open_result) {
    emit error(tr("Unable to connect serial port"));
    qWarning() << "Failed to connect..";
    mutex_.unlock();
    return;
  }
  qInfo() << "Success connect!" << open_result;
  while (current_line_ < gcode_.size()) {
    qInfo() << "Sending " << gcode_[current_line_];
    const QByteArray data = QString(gcode_[current_line_] + "\n").toUtf8();
    serial.write(data);
    qInfo() << "Success write!";

    if (serial.waitForBytesWritten(wait_timeout_)) {
      if (serial.waitForReadyRead(wait_timeout_)) {
        QByteArray resp_data = serial.readAll();
        QString response = QString::fromUtf8(resp_data);
        qInfo() << "Response" << response;
      }
    }
    current_line_++;
  }
  mutex_.unlock();
}