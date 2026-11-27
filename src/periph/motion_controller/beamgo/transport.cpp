#include "transport.h"

#include <QThread>
#include <QDebug>

namespace beamgo {
namespace {
QByteArray byteArray(const Bytes& bytes) {
  return QByteArray(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}
}

Transport::Transport(QObject* parent) : QObject(parent) {
  timeout_.setSingleShot(true);
  connect(&port_, &QSerialPort::readyRead, this, &Transport::receive);
  connect(&port_, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError error) {
    if (error == QSerialPort::ResourceError || error == QSerialPort::DeviceNotFoundError ||
        error == QSerialPort::ReadError || error == QSerialPort::WriteError) fail();
  });
  connect(&timeout_, &QTimer::timeout, this, [this] {
    if (!pending_) return;
    if (pending_->optional && pending_->data[0] == 0x11) complete({}, "UNSUPPORTED");
    else fail();
  });
}

bool Transport::open(const QString& port, qint32 baudRate) {
  Q_ASSERT(QThread::currentThread() == thread());
  port_.setPortName(port);
  port_.setBaudRate(baudRate);
  port_.setDataBits(QSerialPort::Data8);
  port_.setParity(QSerialPort::NoParity);
  port_.setStopBits(QSerialPort::OneStop);
  port_.setFlowControl(QSerialPort::NoFlowControl);
  failed_ = false;
  may_be_running_ = false;
  decoder_.reset();
  consecutive_errors_ = 0;
  if (!port_.open(QIODevice::ReadWrite)) {
    qWarning() << "BeamGo: cannot open" << port << port_.errorString();
    return false;
  }
  port_.setDataTerminalReady(false);
  port_.setRequestToSend(false);
  return isOpen();
}

void Transport::request(Bytes plaintext, Response response, bool optional, bool priority) {
  Q_ASSERT(QThread::currentThread() == thread());
  if (!isOpen()) { response({}, "DISCONNECTED"); return; }
  if (plaintext.size() < 2 || plaintext.size() > 5022 ||
      (optional && plaintext[0] != 0x11)) {
    response({}, "INVALID_PARAMS");
    return;
  }
  Command command{std::move(plaintext), std::move(response), optional};
  if (priority) queue_.push_front(std::move(command));
  else queue_.push_back(std::move(command));
  sendNext();
}

void Transport::sendNext() {
  if (pending_ || queue_.empty() || !isOpen()) return;
  pending_ = std::move(queue_.front());
  queue_.pop_front();
  const auto frame = byteArray(encodeFrame(pending_->data));
  if (pending_->data[0] == 0x12 && (pending_->data[1] == 0 || pending_->data[1] == 4 || pending_->data[1] == 7)) {
    may_be_running_ = true;
  }
  if (port_.write(frame) != frame.size()) { fail(); return; }
  elapsed_.start();
  timeout_.start(750);
}

void Transport::receive() {
  const auto bytes = port_.readAll();
  const size_t old_errors = decoder_.errors();
  const auto messages = decoder_.push(Bytes(bytes.begin(), bytes.end()));
  consecutive_errors_ += decoder_.errors() - old_errors;
  if (consecutive_errors_ >= 3) { fail(); return; }
  if (!messages.empty()) consecutive_errors_ = 0;
  for (const auto& message : messages) {
    if (message.size() < 2) continue;
    if (message[0] == 0x49) {
      Q_EMIT eventReceived(byteArray(message));
    } else if (pending_ && pending_->data[0] == 0x11 && message[0] == 0x11 &&
               pending_->data[1] == message[1]) {
      complete(message);
    } else if (pending_ && pending_->data[0] != 0x11 && message[0] == 0x47) {
      if (message[1] == 0x01) complete(message);
      else if (message[1] == 0x00 && elapsed_.elapsed() < 10000) timeout_.start(750);
      else fail();
    }
  }
}

void Transport::complete(const Bytes& message, const QString& error) {
  timeout_.stop();
  auto response = std::move(pending_->response);
  pending_.reset();
  response(message, error);
  sendNext();
}

void Transport::cancelQueued() {
  auto queued = std::move(queue_);
  queue_.clear();
  for (auto& command : queued) command.response({}, "CANCELLED");
}

void Transport::fail() {
  if (failed_) return;
  failed_ = true;
  timeout_.stop();
  // The acknowledgement stream is no longer trustworthy. Best-effort stops are never retried,
  // and do not imply that the machine stopped; report disconnection and require a new session.
  if (port_.isOpen()) {
    if (may_be_running_) {
      port_.write(byteArray(encodeFrame({0x12, 0x05})));
      port_.write(byteArray(encodeFrame({0x12, 0x01})));
      port_.flush();
    }
    port_.close();
  }
  Q_EMIT disconnected();
  if (pending_) {
    auto response = std::move(pending_->response);
    pending_.reset();
    response({}, "DISCONNECTED");
  }
  cancelQueued();
}

void Transport::close() {
  fail();
}
}  // namespace beamgo
