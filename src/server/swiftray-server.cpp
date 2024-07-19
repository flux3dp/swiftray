#include "swiftray-server.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <cmath>
#include <canvas/canvas.h>
#include <toolpath_exporter/toolpath-exporter.h>

SwiftrayServer::SwiftrayServer(quint16 port, QObject* parent)
  : QObject(parent) {
  this->m_server = new QWebSocketServer("Swiftray Server", QWebSocketServer::NonSecureMode, this);

  m_engrave_dpi = 254;
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
    auto param_str = action == "loadSVG" ? "[skipped]" : params.toString();
    qInfo() << "Received action" << action << "with params" << param_str;
    
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

bool SwiftrayServer::handleParserAction(QWebSocket* socket, const QString& id, const QString& action, const QJsonValue& params) {
  QJsonObject result;
  result["success"] = true;

  if (action == "loadSVG") {
    // Implement BVG data loading logic
    if (m_canvas != nullptr) {
      delete m_canvas;
    }
    m_canvas = new Canvas();
    QJsonObject params_obj = params.toObject();
    QJsonObject wrapped_file = params_obj["file"].toObject();
    QString svg_data = wrapped_file["data"].toString().toUtf8();
    m_rotary_mode = params_obj["rotaryMode"].toBool();
    m_engrave_dpi = params_obj["engraveDpi"].toInt();
    QByteArray svg_data_bytes = QByteArray::fromStdString(svg_data.toStdString());
    m_canvas->loadSVG(svg_data_bytes);
    qInfo() << "SVG data loaded" << svg_data.length();
    result["loadedDataSize"] = svg_data_bytes.length();
  } else if (action == "convert") {
    // Get the parameters
    QJsonObject params_obj = params.toObject();
    QJsonObject workarea = params_obj["workarea"].toObject();
    MachineSettings::MachineParam machine_param;
    machine_param.width = workarea["width"].toInt();
    machine_param.height = workarea["height"].toInt();
    int travel_speed = fmax(params_obj["travelSpeed"].toInt(), 20);
    // Generate GCode
    GCodeGenerator gen(machine_param, m_rotary_mode);
    qInfo() << "Generating GCode..." << "DPI" << m_engrave_dpi << "ROTARY" << m_rotary_mode << "TRAVEL" << travel_speed;
    QTransform move_translate = QTransform();
    auto origin = machine == NULL ? std::make_tuple<qreal, qreal, qreal>(0, 0, 0) : machine->getCustomOrigin();
    ToolpathExporter exporter(
        (BaseGenerator*)&gen,
        m_engrave_dpi / 25.4,
        travel_speed,
        QPointF(std::get<0>(origin), std::get<1>(origin)),
        ToolpathExporter::PaddingType::kFixedPadding,
        move_translate);
    exporter.setSortRule(PathSort::NestedSort);
    exporter.setWorkAreaSize(QRectF(0,0,m_canvas->document().width() / 10, m_canvas->document().height() / 10));

    //
    if ( true != exporter.convertStack(m_canvas->document().layers(), true,  true, nullptr)) {
      return false; // canceled
    }
    if (exporter.isExceedingBoundary()) {
      qWarning() << "Some items aren't placed fully inside the working area.";
    }
    if (m_rotary_mode && m_canvas->calculateShapeBoundary().height() > m_canvas->document().height()) {
      qInfo() << "Rotary mode is enabled, but the height of the design is larger than the working area.";
    }
    m_buffer = QString::fromStdString(gen.toString());
    result["gcode"] = m_buffer;
    result["fileName"] = "swiftray-conversion";
    auto timestamp_list = MachineJob::calcRequiredTime(m_buffer.split("\n"), NULL);
    Timestamp total_required_time{0, 0};
    if (!timestamp_list.empty()) {
      total_required_time = timestamp_list.last();
    }
    result["timeCost"] = total_required_time.second();
    qInfo() << "GCode generation completed." << m_buffer.length() << "time estimate" << result["timeCost"];
    // Debugging GCode
    if (m_buffer.length() < 3000) printf("%s", m_buffer.toStdString().c_str());
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
  // qInfo() << "Sending callback" << msg;
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
    {"version", "5.0.0"},
    {"model", "fpm1"},
    {"port", "/dev/ttyUSB0"},
    {"type", "Galvanometer"},
    {"source", "swiftray"}
  });
  devices.append(QJsonObject{
    {"uuid", "bcf5c788-8635-4ffc-9706-3519d9e8fa7b"},
    {"serial", "LV84KAO192839012"},
    {"name", "Lazervida Origin"},
    {"st_id", 0},
    {"version", "5.0.0"},
    {"model", "flv1"},
    {"port", "/dev/ttyUSB0"},
    {"type", "Laser Cutter"},
    {"source", "swiftray"}
  });
  return devices;
}