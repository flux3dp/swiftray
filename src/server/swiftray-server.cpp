#include "swiftray-server.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

SwiftrayServer::SwiftrayServer(quint16 port, QObject* parent)
  : QObject(parent) {
  this->m_server = new QWebSocketServer("Swiftray Server", QWebSocketServer::NonSecureMode, this);
  if (m_server->listen(QHostAddress::LocalHost, port)) {
    qInfo() << "Swiftray Server listening on port" << port;
    connect(m_server, &QWebSocketServer::newConnection, this, &SwiftrayServer::onNewConnection);
  } else {
    qCritical() << "Failed to start Swiftray Server on port" << port;
  }
}

void SwiftrayServer::onNewConnection() {
  QWebSocket* socket = m_server->nextPendingConnection();
  qInfo() << "New connection from" << socket->peerAddress().toString();
  
  // socket->setOriginHeader("*");
  // socket->setHeaderField("Access-Control-Allow-Origin", "*");
  // socket->setHeaderField("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  // socket->setHeaderField("Access-Control-Allow-Headers", "Content-Type");
  connect(socket, &QWebSocket::textMessageReceived, this, &SwiftrayServer::processMessage);
  connect(socket, &QWebSocket::disconnected, socket, &QWebSocket::deleteLater);
}

void SwiftrayServer::processMessage(const QString& message) {
  QWebSocket* socket = qobject_cast<QWebSocket*>(sender());
  QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
  QJsonObject json = doc.object();

  QString type = json["type"].toString();
  QJsonObject data = json["data"].toObject();

  if (type == "action") {
    QString action = data["action"].toString();
    QString id = data["id"].toString();
    QJsonObject params = data["params"].toObject();
    qInfo() << "Received action" << action << "with id" << id;
    // Process the action based on the path and action name
    // Example:
    if (action == "list") {
      QJsonObject result;
      result["success"] = true;
      result["devices"] = getDeviceList();
      sendCallback(socket, id, result);
    }
    // ... handle other actions
  }
}

void SwiftrayServer::sendCallback(QWebSocket* socket, const QString& id, const QJsonObject& result, const QString& error) {
  QJsonObject callback;
  callback["id"] = id;
  callback["result"] = result;
  callback["type"] = "callback";
  if (error != "") {
    callback["error"] = error;
  }
  QJsonDocument doc(callback);
  QString msg = doc.toJson(QJsonDocument::Compact);
  qInfo() << "Sending callback" << msg;
  socket->sendTextMessage(msg);
}

void SwiftrayServer::sendEvent(QWebSocket* socket, const QString& event, const QJsonObject& data) {
  QJsonObject payload;
  payload["type"] = event;
  payload["data"] = data;
  QJsonDocument doc(payload);
  socket->sendTextMessage(doc.toJson(QJsonDocument::Compact));
}

QJsonArray SwiftrayServer::getDeviceList() {
  QJsonArray devices;
  devices.append(QJsonObject{
    {"uuid", "dcf5c788-8635-4ffc-9706-3519d9e8fa7d"},
    {"name", "Promark Desktop"},
    {"serial", "PD99KJOJIO13993"},
    {"st_id", 0},
    {"model", "promark-d"},
    {"port", "/dev/ttyUSB0"},
    {"type", "Galvanometer"},
    {"source", "swiftray"}
  });
  devices.append(QJsonObject{
    {"uuid", "bcf5c788-8635-4ffc-9706-3519d9e8fa7b"},
    {"serial", "LV84KAO192839012"},
    {"name", "Lazervida Origin"},
    {"st_id", 0},
    {"model", "lazervida"},
    {"port", "/dev/ttyUSB0"},
    {"type", "Laser Cutter"},
    {"source", "swiftray"}
  });
  // Add device information to the array
  return devices;
}