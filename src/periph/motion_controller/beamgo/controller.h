#pragma once

#include "compiler.h"
#include "transport.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QStringList>
#include <memory>

namespace beamgo {
using Reply = std::function<void(QJsonObject, QString)>;

// Owns one physical device and lives on one dedicated Qt thread, including all serial IO.
class Controller : public QObject {
  Q_OBJECT
 public:
  explicit Controller(QString port, QObject* parent = nullptr, qint32 baudRate = 921600);
  void initialize();
  void action(const QString& action, const QJsonObject& params, Reply reply);
  void shutdown();

 Q_SIGNALS:
  void discovered(QJsonObject device);
  void statusChanged(QJsonObject device);
  void disconnected();

 private:
  using Readings = QMap<int, Bytes>;
  using ReadReply = std::function<void(Readings, QString)>;
  QString port_;
  qint32 baud_rate_;
  Transport transport_{this};
  QTimer poll_{this};
  QJsonObject identity_;
  Readings readings_;
  Bytes records_;
  std::vector<Bytes> blocks_;
  size_t next_block_ = 0, uploaded_ = 0, total_ = 0;
  uint32_t processed_ = 0, errors_ = 0, safety_ = 0;
  int state_ = 0, status_ = 0;
  uint64_t generation_ = 0;
  bool ready_ = false, framing_ = false, started_ = false, seen_run_ = false;
  bool busy_ = false, polling_ = false;
  bool stopping_ = false, external_job_ = false;
  QString last_error_;
  Reply start_reply_;
  Reply stop_reply_;

  static void answer(Reply reply, const QString& error = {}, QJsonObject data = {});
  void read(const std::vector<int>& parameters, ReadReply reply, Readings readings = {}, size_t index = 0);
  void refresh();
  void consume(int parameter, const Bytes& body);
  QJsonObject status() const;
  void publish();
  void guard(bool paused, std::function<void(QString)> callback);
  void start(Bytes records, bool framing, Reply reply);
  void stream(uint64_t generation);
  void startMachine(uint64_t generation);
  void stop(Reply reply);
  void waitForIdle(Reply reply, int attempt = 0);
  void calibration(const QString& action, const QJsonObject& params, Reply reply);
  void safetySettings(const QJsonObject& params, Reply reply);
  void uploadProject(const QJsonObject& params, Reply reply);
  void sendFilePackets(std::shared_ptr<std::vector<Bytes>> packets, size_t index,
                       uint64_t generation, Reply reply);
  void writeVerified(int parameter, const Bytes& value, Response callback);
};
}  // namespace beamgo
