#include "worker.h"
#include "swiftray-server.h"
#include <QCoreApplication>
#include <QJsonObject>
#include <executor/machine_job/machine_job.h>
#include <toolpath_exporter/convex-hull-exporter.h>
#include <toolpath_exporter/toolpath-exporter-fcode.h>
#include <toolpath_exporter/toolpath-exporter.h>

SwiftrayServer* server_;
Worker::Worker(QObject* server) {
  server_ = dynamic_cast<SwiftrayServer*>(server);
}

void Worker::handleInterrupt(QPointer<QWebSocket> socket_ptr) {
  if (socket_ptr_.isNull() || socket_ptr_ == socket_ptr) {
    Q_EMIT interruptAction();
    QCoreApplication::processEvents();
  }
}

bool Worker::handleAction(QWebSocket* socket,
                          const QString& id,
                          const QString& action,
                          const QJsonValue& params) {
  QJsonObject result;
  if (!server_->canvas_mutex_.try_lock()) {
    result["success"] = false;
    result["error"] = QJsonObject{{"message", "The backend is currently busy. Please try again later."}},
    Q_EMIT sendCallbackInMain(socket, id, result);
    QCoreApplication::processEvents();
    return false;
  }
  socket_ptr_ = socket;
  result["success"] = true;
  QString current_task = "";
  auto onProgress = [&](int prog) {
    if (socket_ptr_.isNull()) return;
    Q_EMIT sendDataInMain(socket,
                          id,
                          QJsonObject{{"message", current_task},
                                      {"percentage", float(prog) / 100}},
                          "progress");
    QCoreApplication::processEvents();
  };
  auto onCancel = [&]() {
    server_->canvas_mutex_.unlock();
    if (socket_ptr_.isNull()) return;
    result["success"] = false;
    result["error"] = QJsonObject{{"message", "cancel"}},
    Q_EMIT sendCallbackInMain(socket, id, result);
    QCoreApplication::processEvents();
  };

  if (action == "loadSVG") {
    // TODO: add progress and cancel support
    // Implement BVG data loading logic
    if (server_->m_canvas != nullptr) {
      delete server_->m_canvas;
    }
    QJsonObject params_obj = params.toObject();
    QJsonObject wrapped_file = params_obj["file"].toObject();
    QString svg_data = wrapped_file["data"].toString().toUtf8();
    server_->m_canvas = new Canvas();
    server_->m_thumbnail = wrapped_file["thumbnail"].toString();
    server_->m_rotary_mode = params_obj["rotaryMode"].toBool();
    server_->m_engrave_dpi = params_obj["engraveDpi"].toInt();
    QJsonObject default_config = params_obj["defaultConfig"].toObject();
    QByteArray svg_data_bytes = QByteArray::fromStdString(svg_data.toStdString());
    server_->m_canvas->loadSVG(svg_data_bytes, true, default_config);
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
    bool is_promark = params_obj["isPromark"].toBool();
    if (!is_promark) {
      qInfo() << "Generating Task Code... TYPE" << type << "DPI" << server_->m_engrave_dpi;
      QTransform move_translate = QTransform();
      ToolpathExporterFcode exporter(move_translate, server_->m_engrave_dpi, &params_obj, &server_->m_thumbnail);
      current_task = type == "gcode" ? "Generating Task Path..." : "Generating Task Code...";
      onProgress(0);
      connect(this, &Worker::interruptAction, &exporter, &ToolpathExporterFcode::handleCancel, Qt::QueuedConnection);
      connect(&exporter, &ToolpathExporterFcode::progressChanged, onProgress);
      bool completed = exporter.convertStack(server_->m_canvas->document().layers(), nullptr);
      disconnect(&exporter);
      exporter.disconnect();
      if (!completed) {
        onCancel();
        return false;
      }
      if (type == "fcode") {
        QString fcode(QByteArray::fromStdString(exporter.toString()).toBase64());
        int max_chunk_size = 4096;
        if (fcode.length() > max_chunk_size) {
          qInfo() << "FCode is too large, sending as chunks" << fcode.length();
          for (int i = 0; i < fcode.length(); i += max_chunk_size) {
            result["chunk"] = fcode.mid(i, max_chunk_size);
            Q_EMIT sendDataInMain(socket, id, result, "chunk");
          }
          result.remove("chunk");
        } else {
          result["fcode"] = fcode;
        }
        result["timeCost"] = exporter.getTimeCost();
        result["metadata"] = exporter.getMetadata();
      } else if (type == "preview") {
        // Return task time for path preview
        result["timeCost"] = exporter.getTimeCost();
        result["metadata"] = exporter.getMetadata();
      } else {
        result["gcode"] = QString::fromStdString(exporter.toString());
      }
      result["fileName"] = "swiftray-conversion";
    } else {
      qInfo() << "Generating Promark GCode..." << "DPI" << server_->m_engrave_dpi << "ROTARY" << server_->m_rotary_mode << "TRAVEL" << travel_speed;
      bool use_fast_gradient = params_obj["shouldUseFastGradient"].toBool();
      bool enable_high_speed = (server_->m_machine == NULL || server_->m_machine->getMachineParam().is_high_speed_mode) && server_->m_canvas->hasBitmap() && use_fast_gradient;
      // Generate GCode
      GCodeGenerator gen(machine_param, server_->m_rotary_mode);
      if (server_->m_rotary_mode) {
        gen.setRotary(params_obj["spin"].toDouble(0) / 10, // spin in px
                      params_obj["rotary_y_ratio"].toDouble(1),
                      params_obj["rotary_split"].toDouble(0),
                      params_obj["rotary_overlap"].toDouble(0));
      }
      QTransform move_translate = QTransform();
      auto origin = server_->m_machine == nullptr ? std::make_tuple<qreal, qreal, qreal>(0, 0, 0) : server_->m_machine->getCustomOrigin();
      if (type == "hull") {
        ConvexHullExporter exporter((BaseGenerator*)&gen);
        exporter.setWorkAreaSize(QRectF(0, 0, server_->m_canvas->document().width() / 10, server_->m_canvas->document().height() / 10));

        current_task = "Generating Hull Task Code...";
        onProgress(0);
        connect(this, &Worker::interruptAction, &exporter, &ConvexHullExporter::handleCancel, Qt::QueuedConnection);
        connect(&exporter, &ConvexHullExporter::progressChanged, onProgress);
        if (true != exporter.convertStack(server_->m_canvas->document().layers())) {
          onCancel();
          return false; // canceled
        }
        disconnect(&exporter);
        exporter.disconnect();
        if (exporter.isExceedingBoundary()) {
          qWarning() << "Some items aren't placed fully inside the working area.";
        }
      } else {
        ToolpathExporter exporter(
            (BaseGenerator*)&gen,
            server_->m_engrave_dpi / 25.4,
            travel_speed,
            QPointF(std::get<0>(origin), std::get<1>(origin)),
            ToolpathExporter::PaddingType::kNoPadding,
            move_translate,
            true);
        exporter.setSortRule(PathSort::NestedSort);
        exporter.setWorkAreaSize(QRectF(0, 0, server_->m_canvas->document().width() / 10, server_->m_canvas->document().height() / 10));
        if (type == "contour") exporter.handleContour();
        if (params_obj.contains("mask")) exporter.setShouldClipWorkarea(true);

        current_task = "Generating Task Code...";
        onProgress(0);
        connect(this, &Worker::interruptAction, &exporter, &ToolpathExporter::handleCancel, Qt::QueuedConnection);
        connect(&exporter, &ToolpathExporter::progressChanged, onProgress);
        if (true != exporter.convertStack(server_->m_canvas->document().layers(), enable_high_speed, true)) {
          onCancel();
          return false; // canceled
        }
        disconnect(&exporter);
        exporter.disconnect();
        if (exporter.isExceedingBoundary()) {
          qWarning() << "Some items aren't placed fully inside the working area.";
        }
      }
      if (server_->m_rotary_mode && server_->m_canvas->calculateShapeBoundary().height() > server_->m_canvas->document().height()) {
        qInfo() << "Rotary mode is enabled, but the height of the design is larger than the working area.";
      }
      qInfo() << "Conversion completed.";
      server_->m_buffer = QString::fromStdString(gen.toString());
      server_->gcode_list_ = server_->m_buffer.split("\n");
      current_task = "Calculating Task Time...";
      onProgress(0);
      MachineJob job;
      connect(this, &Worker::interruptAction, &job, &MachineJob::handleCancel, Qt::QueuedConnection);
      connect(&job, &MachineJob::progressChanged, this, onProgress);
      server_->m_time_cost = job.calcTotalTime(server_->gcode_list_) / 1000;
      disconnect(&job);
      job.disconnect();
      if (job.cancelled) {
        onCancel();
        return false;
      }
      result["gcode"] = server_->m_buffer;
      result["fileName"] = "swiftray-conversion";
      result["timeCost"] = server_->m_time_cost;
      qInfo() << "GCode generation completed." << server_->m_buffer.length() << "time estimate" << server_->m_time_cost;
    }
  } else {
    result["success"] = false;
    result["error"] = QJsonObject{{"message", "unknown action"}};
  }

  server_->canvas_mutex_.unlock();
  if (socket_ptr_.isNull()) return false;
  Q_EMIT sendCallbackInMain(socket, id, result);
  QCoreApplication::processEvents();
  return true;
}
