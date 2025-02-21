#pragma once

#include <QJsonObject>
#include <QObject>
#include <QWebSocket>

#include <canvas/canvas.h>
#include <toolpath_exporter/generators/gcode-generator.h>

// Handling long-time actions
class Worker : public QObject {
  Q_OBJECT

 public:
  Worker(QObject* parent);
  void handleInterrupt();
  bool handleAction(QWebSocket* socket, const QString& id, const QString& action, const QJsonValue& params);

 Q_SIGNALS:
  void interruptAction();
  void sendDataInMain(QWebSocket* socket, const QString& id, const QJsonObject& result, const QString& type);
  void sendCallbackInMain(QWebSocket* socket, const QString& id, const QJsonObject& result);
};