#include "swiftray-server.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

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
  
  connect(socket, &QWebSocket::textMessageReceived, this, &SwiftrayServer::processMessage);
  connect(socket, &QWebSocket::disconnected, socket, &QWebSocket::deleteLater);
}

void SwiftrayServer::processMessage(const QString& message) {
  QWebSocket* socket = qobject_cast<QWebSocket*>(sender());
  QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
  QJsonObject json = doc.object();

  QString type = json["type"].toString();
  QString path = json["path"].toString();
  QJsonObject data = json["data"].toObject();

  if (type == "action") {
    QString action = data["action"].toString();
    QString id = data["id"].toString();
    QJsonValue params = data["params"];
    qInfo() << "Received action" << action << "with id" << id << "on path" << path;
    
    if (path == "/devices") {
      handleDevicesAction(socket, id, action, params);
    } else if (path.startsWith("/devices/")) {
      handleDeviceSpecificAction(socket, id, action, params, path.mid(9));
    } else if (path == "/parser") {
      handleParserAction(socket, id, action, params);
    } else if (path == "/ws/sr/system") {
      handleSystemAction(socket, id, action, params);
    }
  }
}

void SwiftrayServer::handleDevicesAction(QWebSocket* socket, const QString& id, const QString& action, const QJsonValue& params) {
  if (action == "list") {
    QJsonObject result;
    result["success"] = true;
    result["devices"] = getDeviceList();
    sendCallback(socket, id, result);
  }
}

void SwiftrayServer::handleDeviceSpecificAction(QWebSocket* socket, const QString& id, const QString& action, const QJsonValue& params, const QString& port) {
  QJsonObject result;
  result["success"] = true;

  if (action == "connect") {
    // Implement device connection logic here
    // TODO:SERVER Determine param based on port
    this->machine = new Machine(MachineSettings::MachineParam());
    result["message"] = "Connected to device on port " + port;
  } else if (action == "start") {
    this->machine->startJob();
  } else if (action == "pause") {
    this->machine->pauseJob();
  } else if (action == "resume") {
    this->machine->resumeJob();
  } else if (action == "stop") {
    this->machine->stopJob();
  } else if (action == "getParam") {
    QString paramName = params.toObject()["name"].toString();
    // Implement get device parameter logic, probably laser speed, power, fan...etc
    result["value"] = 0; // Replace with actual value
  } else if (action == "setParam") {
    QString paramName = params.toObject()["name"].toString();
    QJsonValue paramValue = params.toObject()["value"];
    // Implement set device parameter logic
  } else if (action == "getSettings") {
    // Implement get device settings logic
    result["settings"] = QJsonObject(); // Replace with actual settings
  } else if (action == "updateSettings") {
    // Implement update device settings logic
  } else if (action == "deleteSettings") {
    // Implement delete device settings logic
  } else if (action == "updateFirmware") {
    // Implement firmware update logic
  } else if (action == "endMode") {
    // Implement end mode logic
  } else if (action == "switchMode") {
    // Implement switch mode logic
  } else if (action == "quit") {
    // Implement quit task logic
  } else if (action == "downloadLog") {
    // Implement log download logic
  } else if (action == "downloadFile") {
    // Implement file download logic
  } else if (action == "info") {
    // Implement device info retrieval logic
  } else if (action == "getPreview") {
    // Implement preview retrieval logic
  } else if (action == "kick") {
    // Implement kick logic
  } else if (action == "upload") {
    // Implement file upload logic
  } else if (action == "sendGCode") {
    QString gcode = params.toObject()["gcode"].toString();
    Executor* executor = this->machine->getConsoleExecutor().data();
    this->machine->getMotionController()->sendCmdPacket(executor, gcode);
  } else if (action == "getStatus") {
    result["st_id"] = this->machine->getJobExecutor()->getStatusId();
  } else if (action == "home") {
    // Implement homing logic
  } else {
    result["success"] = false;
    result["error"] = "Unknown action";
  }

  sendCallback(socket, id, result);
}

void SwiftrayServer::handleParserAction(QWebSocket* socket, const QString& id, const QString& action, const QJsonValue& params) {
  QJsonObject result;
  result["success"] = true;

  if (action == "loadBvgData") {
    // Implement BVG data loading logic
  } else if (action == "convert") {
    // Implement conversion logic
  } else if (action == "loadSettings") {
    // Implement settings loading logic
  } else {
    result["success"] = false;
    result["error"] = "Unknown parser action";
  }

  sendCallback(socket, id, result);
}

void SwiftrayServer::handleSystemAction(QWebSocket* socket, const QString& id, const QString& action, const QJsonValue& params) {
  QJsonObject result;
  result["success"] = true;

  if (action == "getInfo") {
    QJsonObject info;
    info["swiftrayVersion"] = "1.0.0";
    info["qtVersion"] = QT_VERSION_STR;
    info["os"] = QSysInfo::prettyProductName();
    info["cpuArchitecture"] = QSysInfo::currentCpuArchitecture();
    info["totalMemory"] = 0; // Implement memory retrieval
    info["availableMemory"] = 0; // Implement memory retrieval
    result["info"] = info;
  } else {
    result["success"] = false;
    result["error"] = "Unknown system action";
  }

  sendCallback(socket, id, result);
}

void SwiftrayServer::sendCallback(QWebSocket* socket, const QString& id, const QJsonObject& result) {
  QJsonObject callback;
  callback["id"] = id;
  callback["result"] = result;
  callback["type"] = "callback";
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
  return devices;
}