#pragma once

#include "protocol.h"
#include <QElapsedTimer>
#include <QObject>
#include <QSerialPort>
#include <QTimer>
#include <deque>
#include <functional>
#include <optional>

namespace beamgo {
using Response = std::function<void(const Bytes&, const QString&)>;

class Transport : public QObject {
  Q_OBJECT
 public:
  explicit Transport(QObject* parent = nullptr);
  bool open(const QString& port, qint32 baudRate = 921600);
  bool isOpen() const { return port_.isOpen() && !failed_; }
  void request(Bytes plaintext, Response response, bool optional = false, bool priority = false);
  void cancelQueued();
  void close();
  void observeRunning(bool running) { may_be_running_ = running; }

 Q_SIGNALS:
  void eventReceived(QByteArray plaintext);
  void disconnected();

 private:
  struct Command { Bytes data; Response response; bool optional; };
  QSerialPort port_{this};
  QTimer timeout_{this};
  QElapsedTimer elapsed_;
  FrameDecoder decoder_;
  std::deque<Command> queue_;
  std::optional<Command> pending_;
  bool failed_ = false;
  bool may_be_running_ = false;
  size_t consecutive_errors_ = 0;
  void sendNext();
  void receive();
  void complete(const Bytes& message, const QString& error = {});
  void fail();
};
}  // namespace beamgo
