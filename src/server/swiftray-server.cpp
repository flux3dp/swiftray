#define _HAS_STD_BYTE 0 // This fixes a Windows compatibility issue
#include "liblcs/lcsExpr.h"
#undef _HAS_STD_BYTE
#include "swiftray-server.h"
#include <cmath>
#include <canvas/canvas.h>
#include <toolpath_exporter/generators/dirty-area-outline-generator.h>
#include <toolpath_exporter/toolpath-exporter.h>
#include <toolpath_exporter/toolpath-exporter-fcode.h>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QSerialPortInfo>

SwiftrayServer::SwiftrayServer(quint16 port, QObject* parent)
  : QObject(parent) {
  this->m_server = new QWebSocketServer("Swiftray Server", QWebSocketServer::NonSecureMode, this);

  m_machine = nullptr;
  this->m_engrave_dpi = 512;
  if (m_server->listen(QHostAddress::LocalHost, port)) {
    qInfo() << "Swiftray Server listening on port" << port;
    connect(m_server, &QWebSocketServer::newConnection, this, &SwiftrayServer::onNewConnection);
  } else {
    qCritical() << "Failed to start Swiftray Server on port" << port;
  }
}

Machine* SwiftrayServer::getMachine() {
  if (m_machine == nullptr) {
    throw std::runtime_error("Machine not set");
  }
  return m_machine;
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
    auto param_str = action == "loadSVG" ? "" : params.toString();
    if (param_str != "") {
      qInfo() << QTime::currentTime().toString("HH:mm:ss") << "ws://" + action << "with params" << param_str;
    } else if (action != "getStatus" && action != "list") {
      qInfo() << QTime::currentTime().toString("HH:mm:ss") << "ws://" + action;
    }
    try {
      if (path == "/devices") {
        handleDevicesAction(socket, id, action, params);
      } else if (path.startsWith("/devices/")) {
        handleDeviceSpecificAction(socket, id, action, params, path.mid(9));
      } else if (path == "/parser") {
        handleParserAction(socket, id, action, params);
      } else if (path == "/ws/sr/system") {
        handleSystemAction(socket, id, action, params);
      }
    } catch (std::exception& e) {
      qCritical() << "Error processing action" << action << e.what();
      sendCallback(socket, id, QJsonObject{{"success", false}, {"error", e.what()}});
    } catch (...) {
      qCritical() << "Error processing action" << action;
      sendCallback(socket, id, QJsonObject{{"success", false}, {"error", "Unknown error"}});
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
  qInfo() << "Device Specific action" << action;

  if (action == "connect") {
    if (this->machine_map_.contains(port)) {
      qInfo() << "Server:: Already connected to device on port" << port;
      result["message"] = "Already connected to device on port " + port;
      this->m_machine = this->machine_map_[port];
    } else {
      qInfo() << "Server:: Connecting to device on new port" << port;
      MachineSettings::MachineParam bsl_param;
      bsl_param.name = "Default";
      bsl_param.board_type = MachineSettings::MachineParam::BoardType::BSL_2024;
      bsl_param.origin = MachineSettings::MachineParam::OriginType::RearLeft;
      bsl_param.width = 110;
      bsl_param.height = 110;
      bsl_param.travel_speed = 4000;
      bsl_param.rotary_axis = 'Y';
      bsl_param.home_on_start = false;
      bsl_param.is_high_speed_mode = false;
      this->m_machine = new Machine(bsl_param);
      machine_map_.insert(port, m_machine);
      result["message"] = "Connected to device on port " + port;
      getMachine()->connectSerial("BSL", 0);
    }
  } else if (action == "start") {
    qInfo() << "Starting job";
    getMachine()->startJob();
  } else if (action == "pause") {
    getMachine()->pauseJob();
  } else if (action == "resume") {
    getMachine()->resumeJob();
  } else if (action == "stop") {
    getMachine()->stopJob();
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
    getMachine()->getJobExecutor()->reset();
  } else if (action == "downloadLog") {
    // Implement log download logic
  } else if (action == "downloadFile") {
    // Implement file download logic
  } else if (action == "info") {
    // Implement device info retrieval logic
  } else if (action == "getPreview") {
    // Implement preview retrieval logic
    qInfo() << "SwiftrayServer::getPreview()";
    result["time_cost"] = this->m_time_cost; // Replace with actual time cost
  } else if (action == "kick") {
    // Implement kick logic
  } else if (action == "startFraming") {
    result["success"] = this->startFraming();
  } else if (action == "stopFraming") {
    getMachine()->stopJob();
  } else if (action == "upload") {
    // Implement file upload logic
    qInfo() << "File uploaded";
    bool job_result = getMachine()->createGCodeJob(gcode_list_, timestamp_list_);
    qInfo() << "Job created" << job_result;
    result["success"] = job_result;
  } else if (action == "sendGCode") {
    QString gcode = params.toObject()["gcode"].toString();
    Executor* executor = getMachine()->getConsoleExecutor().data();
    getMachine()->getMotionController()->sendCmdPacket(executor, gcode);
  } else if (action == "getStatus") { // The old "play report" action in Beam Studio
    result["st_id"] = getMachine()->getStatusId();
    result["prog"] = getMachine()->getJobExecutor()->getProgress() * 0.01f; 
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
    QJsonObject params_obj = params.toObject();
    QJsonObject wrapped_file = params_obj["file"].toObject();
    QString svg_data = wrapped_file["data"].toString().toUtf8();
    this->m_canvas = new Canvas();
    this->m_thumbnail = wrapped_file["thumbnail"].toString();
    this->m_rotary_mode = params_obj["rotaryMode"].toBool();
    this->m_engrave_dpi = params_obj["engraveDpi"].toInt();
    QByteArray svg_data_bytes = QByteArray::fromStdString(svg_data.toStdString());
    this->m_canvas->loadSVG(svg_data_bytes);
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
    QString type = params_obj["type"].toString();
    if (type == "fcode") {
      QTransform move_translate = QTransform();
      ToolpathExporterFcode exporter(move_translate, m_engrave_dpi, &params_obj, &m_thumbnail);
      bool completed = exporter.convertStack(m_canvas->document().layers(), nullptr);
      if (!completed) {
        return false;
      }
      result["fcode"] = QString(QByteArray::fromStdString(exporter.toString()).toBase64());
      result["fileName"] = "swiftray-conversion";
      result["timeCost"] = exporter.getTimeCost();
    } else {
    bool enable_high_speed = type == "fcode" && (m_machine == NULL || this->m_machine->getMachineParam().is_high_speed_mode);
    // Generate GCode
    GCodeGenerator gen(machine_param, this->m_rotary_mode);
    qInfo() << "Generating GCode..." << "DPI" << this->m_engrave_dpi << "ROTARY" << this->m_rotary_mode << "TRAVEL" << travel_speed;
    QTransform move_translate = QTransform();
    auto origin = m_machine == nullptr ? std::make_tuple<qreal, qreal, qreal>(0, 0, 0) : m_machine->getCustomOrigin();
    ToolpathExporter exporter(
        (BaseGenerator*)&gen,
        this->m_engrave_dpi / 25.4,
        travel_speed,
        QPointF(std::get<0>(origin), std::get<1>(origin)),
        ToolpathExporter::PaddingType::kFixedPadding,
        move_translate);
    exporter.setSortRule(PathSort::NestedSort);
    exporter.setWorkAreaSize(QRectF(0,0,m_canvas->document().width() / 10, m_canvas->document().height() / 10));

    //
    if ( true != exporter.convertStack(m_canvas->document().layers(), enable_high_speed,  true)) {
      return false; // canceled
    }
    if (exporter.isExceedingBoundary()) {
      qWarning() << "Some items aren't placed fully inside the working area.";
    }
    if (this->m_rotary_mode && m_canvas->calculateShapeBoundary().height() > m_canvas->document().height()) {
      qInfo() << "Rotary mode is enabled, but the height of the design is larger than the working area.";
    }
    qInfo() << "Conversion completed.";
    m_buffer = QString::fromStdString(gen.toString());
    // Write m_buffer to file
    QFile file("swiftray-conversion.gcode");
    if (file.open(QIODevice::WriteOnly)) {
      QTextStream stream(&file);
      stream << m_buffer;
      file.close();
    }
    gcode_list_ = m_buffer.split("\n");
    result["gcode"] = m_buffer;
    result["fileName"] = "swiftray-conversion";
    timestamp_list_ = MachineJob::calcRequiredTime(gcode_list_, nullptr);
    Timestamp total_required_time{0, 0};
    if (!timestamp_list_.empty()) {
      total_required_time = timestamp_list_.last();
    }
    result["timeCost"] = total_required_time.second();
    qInfo() << "GCode generation completed." << m_buffer.length() << "time estimate" << result["timeCost"];
    this->m_time_cost = total_required_time.second();
    // Debugging GCode
    if (m_buffer.length() < 3000) printf("%s", m_buffer.toStdString().c_str());
    }
  } else if (action == "loadSettings") {
    // Implement settings loading logic
  } else {
    result["success"] = false;
    result["error"] = "Unknown parser action";
  }

  sendCallback(socket, id, result);
  return true;
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

bool serialPortAvailable() {
  const auto infos = QSerialPortInfo::availablePorts();
  for (const QSerialPortInfo &info : infos) {
    if(info.portName().startsWith("cu.") || info.portName().startsWith("tty.Bluetooth")) {
      continue;
    }
    return true;
  }
  return false;
}

QJsonArray SwiftrayServer::getDeviceList() {
  QJsonArray devices;
  if (lcs_available()) {
    int st_id = 0;
    float st_prog = 0.0f;
    QString sn = "ABC123";
    if (this->m_machine != nullptr) {
      st_id = this->m_machine->getStatusId();
      sn = this->m_machine->getConfig("serial");
      st_prog = this->m_machine->getJobExecutor()->getProgress() * 0.01f;
    }
    devices.append(QJsonObject{
      {"uuid", "dcf5c788-8635-4ffc-9706-3519d9e8fa7d"},
      {"name", "Promark Desktop"},
      {"serial", sn},
      {"st_id", st_id},
      {"st_prog", st_prog},
      {"version", "5.0.0"},
      {"model", "fpm1"},
      {"port", "/BSL"},
      {"type", "Galvanometer"},
      {"source", "swiftray"}
    });
  }
  if (serialPortAvailable()) {
    int st_id = 0;
    float st_prog = 0.0f;
    QString sn = "ABC123";
    if (this->m_machine != nullptr) {
      st_id = this->m_machine->getStatusId();
      sn = this->m_machine->getConfig("serial");
      st_prog = this->m_machine->getJobExecutor()->getProgress() * 0.01f;
    }
    devices.append(QJsonObject{
      {"uuid", "bcf5c788-8635-4ffc-9706-3519d9e8fa7b"},
      {"serial", "LV84KAO192839012"},
      {"name", "Lazervida Origin"},
      {"st_id", st_id},
      {"st_prog", st_prog},
      {"version", "5.0.0"},
      {"model", "flv1"},
      {"port", "/dev/ttyUSB0"},
      {"type", "Grbl"},
      {"source", "swiftray"}
    });
  }
  return devices;
}

bool SwiftrayServer::startFraming() {
  qInfo() << "Starting framing job";
  if (getMachine()->getJobExecutor()->getActiveJob()) {
    throw std::runtime_error("Job already running");
  }
  if (getMachine()->getConnectionState() != Machine::ConnectionState::kConnected) {
    throw std::runtime_error("Machine not connected");
  }

  // Generate gcode for framing
  QTransform move_translate = QTransform();
  auto origin = m_machine == nullptr ? std::make_tuple<qreal, qreal, qreal>(0, 0, 0) : getMachine()->getCustomOrigin();
  
  DirtyAreaOutlineGenerator outline_generator(getMachine()->getMachineParam(), this->m_rotary_mode);
  Document &doc = m_canvas->document();
  ToolpathExporter exporter(&outline_generator, 
      doc.settings().dpmm(),
      getMachine()->getMachineParam().travel_speed,
      QPointF(std::get<0>(origin), std::get<1>(origin)),
      ToolpathExporter::PaddingType::kNoPadding,
      move_translate);
  exporter.setSortRule(PathSort::NestedSort);
  exporter.setWorkAreaSize(QRectF(0,0,doc.width() / 10, doc.height() / 10)); // TODO: Set machine work area in unit of mm
  exporter.convertStack(doc.layers(),  getMachine()->getMachineParam().is_high_speed_mode,  true);
  throw new std::runtime_error("Some items aren't placed fully inside the working area.");

  outline_generator.setTravelSpeed(getMachine()->getMachineParam().travel_speed);// mm/s to mm/min
  outline_generator.setLaserPower(0.0f);
  // Create Framing Job and start
  if (getMachine()->createFramingJob(QString::fromStdString(outline_generator.toString()).split("\n"))) {
    getMachine()->startJob();
    return true;
  }
  return false;
}