#pragma once

#include <QObject>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>

class SwiftrayServer : public QObject {
  Q_OBJECT

public:
  explicit SwiftrayServer(quint16 port, QObject* parent = nullptr);

private Q_SLOTS:
  void onNewConnection();
  void processMessage(const QString& message);

private:
  QWebSocketServer* m_server;

  void sendCallback(QWebSocket* socket, const QString& id, const QJsonObject& result = QJsonObject(), const QString& error = QString());
  void sendEvent(QWebSocket* socket, const QString& event, const QJsonObject& data);

  QJsonArray getDeviceList();

  void handleDeviceList(QWebSocket* socket, const QString& id);
  void handleDeviceConnect(QWebSocket* socket, const QString& id, const QString& port);
  void handleDeviceStart(QWebSocket* socket, const QString& id, const QString& port, const QJsonObject& params);
  void handleLoadBvgData(QWebSocket* socket, const QString& id, const QJsonObject& params);
  void handleConvert(QWebSocket* socket, const QString& id, const QJsonObject& params);

  bool connectToDevice(const QString& port);
  bool startTask(const QString& port, const QJsonObject& params);
  int getEstimatedDuration();
  bool loadBvgData(const QJsonObject& params);
  bool convert(const QJsonObject& params);
  int getEstimatedTime();
  QString getResultUuid();
  QString getResultType();
};
