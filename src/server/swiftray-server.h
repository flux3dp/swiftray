#pragma once

#include <QObject>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <machine/machine.h>

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

  void handleDevicesAction(QWebSocket* socket, const QString& id, const QString& action, const QJsonValue& params);
  void handleDeviceSpecificAction(QWebSocket* socket, const QString& id, const QString& action, const QJsonValue& params, const QString& port);
  void handleParserAction(QWebSocket* socket, const QString& id, const QString& action, const QJsonValue& params);
  void handleSystemAction(QWebSocket* socket, const QString& id, const QString& action, const QJsonValue& params);
  void sendCallback(QWebSocket* socket, const QString& id, const QJsonObject& result);
  void sendEvent(QWebSocket* socket, const QString& event, const QJsonObject& data);
  QJsonArray getDeviceList();
};