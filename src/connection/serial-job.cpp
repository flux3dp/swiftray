#include <connection/serial-job.h>
#include <QIODevice>
#include <QMessageBox>
#include <QDebug>
#include <QSerialPortInfo>
#include <QTimer>

void SerialJob::run() {
  mutex_.lock();
  serial_ = new QSerialPort();
  serial_->setPortName(port_);
  serial_->setBaudRate(baudrate_);
  serial_->close();

  connect(serial_, &QSerialPort::readyRead, this, &SerialJob::receive);

  current_line_ = 0;
  block_buffer_ = 0;
  unprocssed_response_.clear();

  // Connecting serial port
  qInfo() << "[SerialPort] Connecting" << port_ << baudrate_;
  auto open_result = serial_->open(QIODevice::ReadWrite);
  if (!open_result) {
    emit error(tr("Unable to connect serial port"));
    qWarning() << "[SerialPort] Failed to connect..";
    mutex_.unlock();
    return;
  }
  qInfo() << "[SerialPort] Success connect!";
  serial_->waitForBytesWritten(wait_timeout_);
  serial_->write("$\n");
  serial_->write("$\n");
  emit startConnection();
  while (!grbl_ready_) {
    serial_->waitForReadyRead(wait_timeout_);
  }
  emit successConnection();
  // Loop sending
  while (current_line_ < gcode_.size()) {
    const QByteArray data = QString(gcode_[current_line_] + "\n").toUtf8();
    block_buffer_++;
    qInfo() << "[SerialPort] Write" << gcode_[current_line_] << "with block buffer" << block_buffer_;
    serial_->write(data);
    serial_->waitForBytesWritten(wait_timeout_);
    serial_->waitForReadyRead(wait_timeout_);
    current_line_++;
  }
  qInfo() << "[SerialPort] All sent - closing";
  serial_->close();
  mutex_.unlock();
}

void SerialJob::receive() {
  QByteArray resp_data = unprocssed_response_ + serial_->readAll();
  QString resp_str = QString::fromUtf8(resp_data);
  int processed_chars = 0;
  for (int i = 0; i < resp_str.length(); i++) {
    if (resp_str[i] == "\n") {
      parseResponse(resp_str.right(resp_str.length() - processed_chars).left(i - processed_chars));
      processed_chars = i + 1;
    }
  }
  unprocssed_response_ = resp_data.right(resp_data.length() - processed_chars);
}

void SerialJob::parseResponse(QString line) {
  if (line.isEmpty()) return;
  qInfo() << "[SerialPort] Receive line" << line;
  if (line == "ok" || line == "ok\r") {
    block_buffer_--;
  } else if (line.startsWith("[HLP:$$")) { // Standard GRBL Response
    grbl_ready_ = true;
    qInfo() << "[SerialPort] Grbl is ready";
  } else {
    qInfo() << "[SerialPort] Unrecognized response";
  }
}

void SerialJob::startTimer() {
  timeout_timer_->start(kGrblTimeout);
}

void SerialJob::stopTimer() {
  timeout_timer_->stop();
}

void SerialJob::timeout() {
  timeout_timer_->stop();
  gcode_.clear();
  grbl_ready_ = true;
  qInfo() << "[SerialPort] connection timeout";
  emit error(tr("GRBL connection timeout"));
}