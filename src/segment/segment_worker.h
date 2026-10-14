#pragma once

#include <QJsonObject>
#include <QJsonValue>
#include <QObject>
#include <QPointer>
#include <QWebSocket>

#include <memory>

namespace segment {
class Sam;
}

// Hosts the MobileSAM ONNX model on its own thread (see SwiftrayServer::setupWorker).
// Path "/segment", actions: info | detect | prompt | unload. Image coordinates in
// replies are pixels of the uploaded image. Contract: beam-studio
// docs/prd/onnx-contour-detection.md §5.2.
class SegmentWorker : public QObject {
  Q_OBJECT

 public:
  explicit SegmentWorker(QObject* parent = nullptr);
  ~SegmentWorker() override;
  void handleAction(QWebSocket* socket, const QString& id, const QString& action, const QJsonValue& params);

 Q_SIGNALS:
  void sendDataInMain(QWebSocket* socket, const QString& id, const QJsonObject& result, const QString& type);
  void sendCallbackInMain(QWebSocket* socket, const QString& id, const QJsonObject& result);

 private:
  QJsonObject info() const;
  QJsonObject detect(const QPointer<QWebSocket>& socket, const QString& id, const QJsonObject& params);
  QJsonObject prompt(const QJsonObject& params);
  bool ensureLoaded(QString* error);
  void unloadIfIdle();
  static QString modelsDir();

  std::unique_ptr<segment::Sam> sam_;
  qint64 last_used_ms_ = 0;
  static constexpr qint64 kIdleUnloadMs = 5 * 60 * 1000;  // free the ~560 MB fp32 session when unused
};
