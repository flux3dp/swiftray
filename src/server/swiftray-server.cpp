#define _HAS_STD_BYTE 0 // This fixes a Windows compatibility issue
#include "liblcs/lcsExpr.h"
#undef _HAS_STD_BYTE
#include "swiftray-server.h"
#include "worker.h"
#include <cmath>
#include <canvas/canvas.h>
#include <toolpath_exporter/generators/dirty-area-outline-generator.h>
#include <toolpath_exporter/toolpath-exporter.h>
#include <toolpath_exporter/toolpath-exporter-fcode.h>
#include <QCoreApplication>
#include <QCryptographicHash>                                                                                                                                                                             
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QSerialPortInfo>
#include <periph/motion_controller/bsl_motion_controller.h>

SwiftrayServer::SwiftrayServer(quint16 port, QObject* parent)
  : QObject(parent) {
  this->m_server = new QWebSocketServer("Swiftray Server", QWebSocketServer::NonSecureMode, this);

  m_machine = nullptr;
  if (m_server->listen(QHostAddress::LocalHost, port)) {
    qInfo() << "Swiftray Server listening on port" << port;
    connect(m_server, &QWebSocketServer::newConnection, this, &SwiftrayServer::onNewConnection);
  } else {
    qCritical() << "Failed to start Swiftray Server on port" << port;
  }
  setupWorker();
  // Set env to avoid Path truncated in parsePathDataFast
  qputenv("QT_SVG_ASSUME_TRUSTED_SOURCE", "1");
}

Machine* SwiftrayServer::getMachine() {
  // TODO: Support other machine parameters
  if (m_machine == nullptr) {
    MachineSettings::MachineParam bsl_param;
    bsl_param.name = "Default";
    bsl_param.board_type = MachineSettings::MachineParam::BoardType::BSL_2024;
    bsl_param.origin = MachineSettings::MachineParam::OriginType::RearLeft;
    bsl_param.width = 110;
    bsl_param.height = 110;
    bsl_param.travel_speed = 4000;
    bsl_param.rotary_axis = 'Y';
    bsl_param.home_on_start = false;
    bsl_param.is_high_speed_mode = true;
    this->m_machine = new Machine(bsl_param);
    this->m_machine->connectSerial("BSL", 0);
    return this->m_machine;
  }
  if (!m_machine->isConnected()) {
    qWarning() << "Server::getMachine(): Device is disconnected, retry connection";
    this->m_machine = new Machine(m_machine->getMachineParam());
    if (!m_machine->connectSerial("BSL", 0)) {
      qWarning() << "Server::getMachine(): Unable to reconnect to machine";
    } else {
      qInfo() << "Server::getMachine(): Reconnected to machine";
      // Sleep for a bit to allow the machine to connect and init
      QThread::msleep(50);
    }
  }
  return m_machine;
}

void SwiftrayServer::onNewConnection() {
  QWebSocket* socket = m_server->nextPendingConnection();
  QPointer<QWebSocket> socket_ptr = socket;
  qInfo() << "New connection from" << socket->peerAddress().toString();
  
  connect(socket, &QWebSocket::textMessageReceived, this, &SwiftrayServer::processMessage);
  connect(socket, &QWebSocket::binaryMessageReceived, this, &SwiftrayServer::processBinaryMessage);
  connect(socket, &QWebSocket::disconnected, socket, &QWebSocket::deleteLater);
  connect(socket, &QWebSocket::disconnected, [&, socket_ptr]() {
    if (workerThread != nullptr) {
      // Note: socket object maybe deleted worker handling the interrupt
      // Use QPointer to avoid accessing deleted object
      Q_EMIT interruptWorker(socket_ptr);
      QCoreApplication::processEvents();
    }
  });
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

void SwiftrayServer::processBinaryMessage(const QByteArray& message) {
  QString text = QString::fromUtf8(message);
  processMessage(text);
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
      getMachine();
      machine_map_.insert(port, this->m_machine);
    } else {
      qInfo() << "Server:: Connecting to device on new port" << port;
      // Create a new machine and write it to the machine map
      this->m_machine = nullptr;
      getMachine();
      machine_map_.insert(port, this->m_machine);
      result["message"] = "Connected to device on port " + port;
    }
  } else if (action == "start") {
    qInfo() << "Server::Starting job";
    double taskTime = params.toObject()["taskTime"].toDouble();
    if (taskTime > 0) {
      BSLMotionController* controller = static_cast<BSLMotionController*>(getMachine()->getMotionController().data());
      controller->setTaskTime(taskTime * 1000);
    }
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
  } else if (action == "setCorrection") {
    double scaleX = params.toObject()["scaleX"].toDouble();
    double scaleY = params.toObject()["scaleY"].toDouble();
    double bucketX = params.toObject()["bucketX"].toDouble();
    double bucketY = params.toObject()["bucketY"].toDouble();
    double paralleX = params.toObject()["paralleX"].toDouble();
    double paralleY = params.toObject()["paralleY"].toDouble();
    double trapeX = params.toObject()["trapeX"].toDouble();
    double trapeY = params.toObject()["trapeY"].toDouble();
    getMachine()->setCorrection(scaleX, scaleY, bucketX, bucketY, paralleX, paralleY, trapeX, trapeY); 
  } else if (action == "setScanaheadParams") {
    QJsonObject obj = params.toObject();
    double worksize = obj["worksize"].toDouble();
    double angle = obj["angle"].toDouble();
    double xOffset = obj["xOffset"].toDouble();
    double yOffset = obj["yOffset"].toDouble();
    getMachine()->setScanaheadParams(worksize, angle, xOffset, yOffset);
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
    result["success"] = this->startFraming(params);
  } else if (action == "stopFraming") {
    getMachine()->stopJob();
  } else if (action == "upload") {
    // Implement file upload logic
    qInfo() << "File uploaded";
    auto obj = params.toObject();
    QString data = obj["data"].toString();
    if (data != "") {
      gcode_list_ = data.split("\n");
    }
    bool job_result = getMachine()->createGCodeJob(gcode_list_, QList<Timestamp>());
    qInfo() << "Job created" << job_result;
    BSLMotionController* controller = static_cast<BSLMotionController*>(getMachine()->getMotionController().data());
    controller->setTaskTime(0);
    if (obj.contains("checkDoor")) {
      bool check_door = obj["checkDoor"].toBool();
      controller->setCheckDoor(check_door);
    }
    result["success"] = job_result;
  } else if (action == "sendGCode") {
    QString gcode = params.toObject()["gcode"].toString();
    Executor* executor = getMachine()->getConsoleExecutor().data();
    getMachine()->getMotionController()->sendCmdPacket(executor, gcode);
  } else if (action == "getStatus") { // The old "play report" action in Beam Studio
    int status_id = getMachine()->getStatusId();
    result["st_id"] = status_id;
    result["prog"] = getMachine()->getJobExecutor()->getProgress() * 0.01f;
    BSLMotionController* controller = static_cast<BSLMotionController*>(getMachine()->getMotionController().data());
    if (controller) {
      if (!controller->isFraming()) {
        result["prog"] = controller->getProgressByTime();
      }
      if (!controller->getBoardStatus().bConnected) {
        result["error"] = "DISCONNECTED";
        if (controller->isHandlingReconnection()) {
          result["st_id"] = 516;
        }
      } else {
        if (controller->isPreparingFirstList() && status_id == 16) {
          result["st_id"] = 1;
        }
        QString error = controller->getCurrentError();
        QStringList errors = error.split(",");
        if (errors.size() > 1) {
          QJsonArray errorArray;
          for(const QString& error : errors) {
            errorArray.append(error);
          }
          result["error"] = errorArray;
        } else {
          result["error"] = error;
        }
      }
      result["disconnection"] = controller->getDisconnectCount();
    } else {
      result["st_id"] = 128;
      result["error"] = "DISCONNECTED";
      result["disconnection"] = -1;
    }
  } else if (action == "home") {
    // Implement homing logic
  } else if (action == "checkButton") {
    uint32_t io_port = lcs_read_io_port();
    result["pressed"] = !(io_port & 0b10);
    BSLMotionController* controller = static_cast<BSLMotionController*>(getMachine()->getMotionController().data());
    if (controller) {
      result["isRunning"] = controller->isRunningLaser();
      result["isFraming"] = controller->isFraming();
    }
  } else {
    result["success"] = false;
    result["error"] = "Unknown action";
  }

  sendCallback(socket, id, result);
}

bool SwiftrayServer::handleParserAction(QWebSocket* socket, const QString& id, const QString& action, const QJsonValue& params) {
  QJsonObject result;
  result["success"] = true;

  if (action == "interrupt") {
    if (workerThread != nullptr) {
      QPointer<QWebSocket> socket_ptr = socket;
      Q_EMIT interruptWorker(socket_ptr);
      QCoreApplication::processEvents();
    }
  } else if (action == "loadSVG" || action == "convert") {
    Q_EMIT sendTaskToWorker(socket, id, action, params);
    QCoreApplication::processEvents();
    return true;
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
    info["swiftrayVersion"] = QString("%1.%2.%3").arg(VERSION_MAJOR).arg(VERSION_MINOR).arg(VERSION_BUILD);
    info["devVersion"] = "4";
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

void SwiftrayServer::sendData(QWebSocket* socket, const QString& id, const QJsonObject& result, const QString& type) {
  if (!socket->isValid()) return;
  QJsonObject payload;
  payload["id"] = id;
  payload["result"] = result;
  payload["type"] = type;
  QJsonDocument doc(payload);
  QString msg = doc.toJson(QJsonDocument::Compact);
  socket->sendTextMessage(msg);
  socket->flush();
}

void SwiftrayServer::sendCallback(QWebSocket* socket, const QString& id, const QJsonObject& result) {
  if (!socket->isValid()) return;
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
  if (!socket->isValid()) return;
  QJsonObject payload;
  payload["type"] = event;
  payload["data"] = data;
  QJsonDocument doc(payload);
  socket->sendTextMessage(doc.toJson(QJsonDocument::Compact));
}

bool serialPortAvailable() {
  const auto infos = QSerialPortInfo::availablePorts();
  for (const QSerialPortInfo &info : infos) {
    if(info.portName().startsWith("cu.") || 
       info.portName().startsWith("tty.Bluetooth") || 
       info.portName().startsWith("tty.BLTH")) {
      continue;
    }
    return true;
  }
  return false;
}

QString hashSerialNumber(const QString& serialNumber) {
  QByteArray hash = QCryptographicHash::hash(serialNumber.toUtf8(),
                                             QCryptographicHash::Sha256);
  return QString(hash.toHex());
}

QJsonArray SwiftrayServer::getDeviceList() {
  QJsonArray devices;
  if (lcs_available()) {
    int st_id = 0;
    float st_prog = 0.0f;
    QString sn = "ABC123";
    QString hashed_sn = "";
    if (this->m_machine != nullptr) {
      st_id = this->m_machine->getStatusId();
      sn = this->m_machine->getConfig("serial");
      hashed_sn = hashSerialNumber(sn);
      st_prog = this->m_machine->getJobExecutor()->getProgress() * 0.01f;
    }
    devices.append(QJsonObject{
      {"uuid", "dcf5c788-8635-4ffc-9706-3519d9e8fa7d"},
      {"name", "Promark"},
      {"serial", sn},
      {"hashed_serial", hashed_sn},
      {"st_id", st_id},
      {"st_prog", st_prog},
      {"version", "5.0.0"},
      {"model", "fpm1"},
      {"port", "/BSL"},
      {"type", "Galvanometer"},
      {"source", "swiftray"}
    });
  }
  return devices;
}

// TODO: split to 'generate task' and 'start job'
// And move generation part to worker for AreaCheck framing
bool SwiftrayServer::startFraming(const QJsonValue& params) {
  if (getMachine()->getJobExecutor()->getActiveJob()) {
    throw std::runtime_error("Job already running");
  }
  if (getMachine()->getConnectionState() != Machine::ConnectionState::kConnected) {
    throw std::runtime_error("Machine not connected");
  }

  QString framing_code = params.toObject()["taskCode"].toString();
  QJsonArray points = params.toObject()["points"].toArray();
  int points_size = points.size();
  int width = params.toObject()["width"].toInt();
  QJsonObject rotary = params.toObject()["rotaryInfo"].toObject();
  bool rotary_mode = points_size == 0 ? this->m_rotary_mode : !rotary.empty();
  DirtyAreaOutlineGenerator outline_generator(getMachine()->getMachineParam(), rotary_mode);
  if (framing_code == "") {
    if (points_size == 0) {
      // Generate gcode for framing by doc
      QTransform move_translate = QTransform();
      auto origin = m_machine == nullptr ? std::make_tuple<qreal, qreal, qreal>(0, 0, 0) : getMachine()->getCustomOrigin();
    
      Document &doc = m_canvas->document();
      ToolpathExporter exporter(&outline_generator, 
          doc.settings().dpmm(),
          getMachine()->getMachineParam().travel_speed,
          QPointF(std::get<0>(origin), std::get<1>(origin)),
          ToolpathExporter::PaddingType::kNoPadding,
          move_translate,
          true);
      exporter.setSortRule(PathSort::NestedSort);
      exporter.setWorkAreaSize(QRectF(0,0,doc.width() / 10, doc.height() / 10)); // TODO: Set machine work area in unit of mm
      exporter.convertStack(doc.layers(),  getMachine()->getMachineParam().is_high_speed_mode,  true);
      if (exporter.isExceedingBoundary()) {
        throw std::runtime_error("Some items aren't placed fully inside the working area.");
      }
    } else {
      // Generate gcode for framing by given points
      if (width > 0) outline_generator.setWorkarea(QRectF(0, 0, width, width));
      for (int i = 0; i < points_size; i++) {
        QJsonArray point = points[i].toArray();
        outline_generator.update_boundary(point[0].toDouble(), point[1].toDouble());
      }
      if (!rotary.empty()) {
        outline_generator.setRotary(rotary["y"].toDouble(0), // y in mm
                                    rotary["yRatio"].toDouble(1),
                                    rotary["ySplit"].toDouble(0),
                                    rotary["yOverlap"].toDouble(0));
      }
    }
    outline_generator.setTravelSpeed(getMachine()->getMachineParam().travel_speed);
    outline_generator.setLaserPower(0.0);
    outline_generator.setStep(10);
    framing_code = QString::fromStdString(outline_generator.toString());
  }
  // Create Framing Job and start
  BSLMotionController* controller = static_cast<BSLMotionController*>(getMachine()->getMotionController().data());
  controller->setTaskTime(0);
  controller->setCheckDoor(false);
  if (getMachine()->createFramingJob(framing_code.split("\n"), !rotary_mode)) {
    getMachine()->startJob();
    return true;
  }
  return false;
}

void SwiftrayServer::setupWorker() {
  if (workerThread == nullptr) {
    qInfo() << "Setup worker thread";
    workerThread = new QThread(this);
    worker = new Worker(this);

    connect(this, &SwiftrayServer::interruptWorker, worker, &Worker::handleInterrupt, Qt::QueuedConnection);
    connect(this, &SwiftrayServer::sendTaskToWorker, worker, &Worker::handleAction, Qt::QueuedConnection);
    connect(worker, &Worker::sendDataInMain, this, &SwiftrayServer::sendData, Qt::QueuedConnection);
    connect(worker, &Worker::sendCallbackInMain, this, &SwiftrayServer::sendCallback, Qt::QueuedConnection);

    worker->moveToThread(workerThread);
    workerThread->start();
  }
}
