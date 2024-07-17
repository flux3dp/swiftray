#pragma once

#include <QObject>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <machine/machine.h>
#include <canvas/canvas.h>
#include <toolpath_exporter/generators/gcode-generator.h>

class SwiftrayServer : public QObject {
  Q_OBJECT

public:
  explicit SwiftrayServer(quint16 port, QObject* parent = nullptr);

private Q_SLOTS:
  void onNewConnection();
  void processMessage(const QString& message);

private:
  QWebSocketServer* m_server;
  Machine* machine;
  QString m_buffer;
  Canvas* m_canvas;

  bool m_rotary_mode;
  int m_engrave_dpi;

  void handleDevicesAction(QWebSocket* socket, const QString& id, const QString& action, const QJsonValue& params);
  void handleDeviceSpecificAction(QWebSocket* socket, const QString& id, const QString& action, const QJsonValue& params, const QString& port);
  bool handleParserAction(QWebSocket* socket, const QString& id, const QString& action, const QJsonValue& params);
  void handleSystemAction(QWebSocket* socket, const QString& id, const QString& action, const QJsonValue& params);
  void sendCallback(QWebSocket* socket, const QString& id, const QJsonObject& result);
  void sendEvent(QWebSocket* socket, const QString& event, const QJsonObject& data);
  QJsonArray getDeviceList();
};