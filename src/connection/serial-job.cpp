#include <connection/serial-job.h>
#include <QIODevice>
#include <QMessageBox>
#include <QDebug>
#include <QSerialPortInfo>
#include <QTimer>

SerialJob::SerialJob(QObject *parent, QString endpoint, QVariant gcode) :
     BaseJob(parent, endpoint, gcode) {
  port_ = endpoint.split(":")[0];
  baudrate_ = endpoint.split(":")[1].toInt();
  gcode_ = gcode.toStringList();
  timeout_timer_ = new QTimer(this);
  grbl_ready_ = false;
  pause_flag_ = false;
  resume_flag_ = false;
  on_hold_ = false;
  connect(this, &SerialJob::startConnection, this, &SerialJob::startTimer);
  connect(this, &SerialJob::successConnection, this, &SerialJob::stopTimer);
  connect(timeout_timer_, &QTimer::timeout, this, &SerialJob::timeout);
}

SerialJob::~SerialJob() {
  mutex_.lock();
  mutex_.unlock();
  wait(wait_timeout_);
}

void SerialJob::start() {
  const QMutexLocker locker(&mutex_);
  if (!isRunning()) {
    QThread::start();
  } else {
    Q_ASSERT_X(false, "SerialJob", "Already running");
  }
}

int SerialJob::progress() {
  return progressValue_;
}

void SerialJob::pause() {
  pause_flag_ = true;
}

void SerialJob::resume() {
  resume_flag_ = true;
}

void SerialJob::run() {
  mutex_.lock();
  serial_ = new QSerialPort();
  serial_->setPortName(port_);
  serial_->setBaudRate(baudrate_);
  serial_->close();

  connect(serial_, &QSerialPort::readyRead, this, &SerialJob::receive);

  current_line_ = 0;
  unprocssed_response_.clear();

  // Connecting serial port
  qInfo() << "[SerialPort] Connecting" << port_ << baudrate_;
  auto open_result = serial_->open(QIODevice::ReadWrite);
  if (!open_result) {
    emit error(tr("Unable to connect serial port"));
    qWarning() << "[SerialPort] Failed to connect..";
    mutex_.unlock();
    setStatus(Status::ERROR_STOPPED);
    return;
  }
  qInfo() << "[SerialPort] Success connect!";
  serial_->waitForBytesWritten(wait_timeout_);
  serial_->write("$\n$\n");
  serial_->flush();
  emit startConnection();
  while (!grbl_ready_) {
    serial_->waitForReadyRead(wait_timeout_);
  }
  emit successConnection();
  setStatus(Status::RUNNING);
  block_buffer_ = 0;
  // Loop sending
  while (current_line_ < gcode_.size()) {
    if (pause_flag_) {
      setStatus(Status::PAUSED);
      serial_->write("!\n");
      serial_->waitForReadyRead(wait_timeout_);
      qInfo() << "[SerialPort] Write Pause (M5!?)";
      block_buffer_ += 1;
      pause_flag_ = false;
      on_hold_ = true;
    } else if (resume_flag_) {
      setStatus(Status::RUNNING);
      serial_->write("~\n");
      serial_->waitForReadyRead(wait_timeout_);
      qInfo() << "[SerialPort] Write ~";
      block_buffer_ += 1;
      resume_flag_ = false;
      on_hold_ = false;
    }
    if (on_hold_) continue;
    const QByteArray data = QString(gcode_[current_line_] + "\n").toUtf8();
    block_buffer_++;
    qInfo() << "[SerialPort] Write" << gcode_[current_line_] << "with block buffer" << block_buffer_;
    serial_->write(data);
    serial_->waitForBytesWritten(wait_timeout_);
    serial_->waitForReadyRead(wait_timeout_);
    current_line_++;
    progressValue_ = (int)(100 * current_line_ / gcode_.size());

    emit progressChanged();
  }
  while (block_buffer_ > 0) {
    qInfo() << "[SerialPort] All sent - waiting buffer to be cleared - left" << block_buffer_;
    serial_->waitForReadyRead(wait_timeout_);
  }
  qInfo() << "[SerialPort] All sent - closing";
  serial_->close();
  mutex_.unlock();
  setStatus(Status::FINISHED);
}

void SerialJob::receive() {
  QByteArray new_data = serial_->readAll();
  //qInfo() << "[SerialPort] Receive data" << new_data;
  QByteArray resp_data = unprocssed_response_ + new_data;
  QString resp_str = QString::fromUtf8(resp_data);
  int processed_chars = 0;
  for (int i = 0; i < resp_str.length(); i++) {
    if (resp_str[i] == '\n') {
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
  } else if (line.startsWith("[MSG:Restoring")) {
    // Do nothing currently
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
