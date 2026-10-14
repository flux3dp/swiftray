#include "segment_worker.h"

#include <QBuffer>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QJsonArray>
#include <QTimer>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <chrono>

#include "detect.h"
#include "sam.h"

namespace {

QJsonArray polygonToJson(const std::vector<std::pair<float, float>>& polygon) {
  QJsonArray arr;
  for (const auto& p : polygon) arr.append(QJsonArray{p.first, p.second});
  return arr;
}

// Orientation of the object's minimum-area rectangle, radians. Both contour engines
// expose this field so the frontend can treat them alike.
double polygonAngleRad(const std::vector<std::pair<float, float>>& polygon) {
  if (polygon.size() < 3) return 0.0;
  std::vector<cv::Point2f> pts;
  pts.reserve(polygon.size());
  for (const auto& p : polygon) pts.emplace_back(p.first, p.second);
  return cv::minAreaRect(pts).angle * CV_PI / 180.0;
}

QJsonObject objectToJson(const segment::DetectedObject& o, int id) {
  return QJsonObject{
      {"id", id},
      {"score", o.score},
      {"area", o.area},
      {"center", QJsonArray{o.cx, o.cy}},
      {"bbox", QJsonArray{o.bx, o.by, o.bw, o.bh}},
      {"angle", polygonAngleRad(o.polygon)},
      {"polygon", polygonToJson(o.polygon)},
  };
}

QJsonObject errorResult(const QString& message) {
  return QJsonObject{{"success", false}, {"error", QJsonObject{{"message", message}}}};
}

}  // namespace

SegmentWorker::SegmentWorker(QObject* parent) : QObject(parent) {}
SegmentWorker::~SegmentWorker() = default;

QString SegmentWorker::modelsDir() {
  QByteArray env = qgetenv("SWIFTRAY_MODELS_DIR");
  if (!env.isEmpty()) return QString::fromUtf8(env);
#ifdef Q_OS_MAC
  return QCoreApplication::applicationDirPath() + "/../Resources/models";
#else
  return QCoreApplication::applicationDirPath() + "/models";
#endif
}

bool SegmentWorker::ensureLoaded(QString* error) {
  if (sam_) return true;
  QDir dir(modelsDir());
  QString enc = dir.filePath("mobile_sam.encoder.onnx");
  QString dec = dir.filePath("mobile_sam.decoder.onnx");
  if (!QFileInfo::exists(enc) || !QFileInfo::exists(dec)) {
    if (error) *error = "Segmentation models not found in " + dir.absolutePath();
    return false;
  }
  try {
    qInfo() << "segment: loading models from" << dir.absolutePath();
    sam_ = std::make_unique<segment::Sam>(enc.toStdString(), dec.toStdString());
  } catch (const std::exception& e) {
    if (error) *error = QString("Failed to load segmentation models: ") + e.what();
    return false;
  }
  return true;
}

void SegmentWorker::unloadIfIdle() {
  if (sam_ && QDateTime::currentMSecsSinceEpoch() - last_used_ms_ >= kIdleUnloadMs) {
    qInfo() << "segment: unloading idle models";
    sam_.reset();
  }
}

QJsonObject SegmentWorker::info() const {
  QDir dir(modelsDir());
  bool available = QFileInfo::exists(dir.filePath("mobile_sam.encoder.onnx")) &&
                   QFileInfo::exists(dir.filePath("mobile_sam.decoder.onnx"));
  return QJsonObject{{"success", true}, {"available", available}, {"loaded", sam_ != nullptr}, {"ep", "cpu"}};
}

QJsonObject SegmentWorker::detect(const QPointer<QWebSocket>& socket, const QString& id, const QJsonObject& params) {
  QString image_b64 = params["image"].toString();
  int comma = image_b64.indexOf(',');
  if (image_b64.startsWith("data:") && comma > 0) image_b64 = image_b64.mid(comma + 1);  // tolerate data URLs
  QImage qimg = QImage::fromData(QByteArray::fromBase64(image_b64.toLatin1()));
  if (qimg.isNull()) return errorResult("Could not decode image");

  segment::Image frame = segment::imageFromQImage(qimg);
  segment::EverythingDetector detector;
  QJsonArray grid = params["grid"].toArray();
  if (grid.size() == 2) {
    detector.grid_x = (std::max)(1, grid[0].toInt());
    detector.grid_y = (std::max)(1, grid[1].toInt());
  }
  if (params.contains("maxObjects")) detector.max_objects = (std::max)(1, params["maxObjects"].toInt());
  detector.on_progress = [&](int done, int total) {
    if (socket.isNull()) return;
    Q_EMIT sendDataInMain(socket.data(), id, QJsonObject{{"done", done}, {"total", total}}, "progress");
  };

  std::vector<segment::DetectedObject> objects = detector.detect(*sam_, frame);
  QJsonArray arr;
  for (size_t i = 0; i < objects.size(); i++) {
    const auto& o = objects[i];
    // one line per returned object so a false positive can be traced back to the filter margins
    qDebug().nospace() << "segment: obj " << i + 1 << " bbox " << o.bx << "," << o.by << " " << o.bw << "x" << o.bh
                       << " area " << o.area << " score " << o.score << " stability " << o.stability << " border " << o.border
                       << " edge " << o.edge_ratio;
    arr.append(objectToJson(o, (int)i + 1));
  }
  return QJsonObject{{"success", true},
                     {"width", frame.w},
                     {"height", frame.h},
                     {"timeMs", detector.last_ms},
                     {"objects", arr}};
}

QJsonObject SegmentWorker::prompt(const QJsonObject& params) {
  if (!sam_->has_embedding()) return errorResult("No image encoded yet - call detect first");
  std::vector<std::pair<float, float>> points;
  std::vector<float> labels;
  for (const QJsonValue& v : params["points"].toArray()) {
    QJsonArray p = v.toArray();
    if (p.size() == 2) points.push_back({(float)p[0].toDouble(), (float)p[1].toDouble()});
  }
  for (const QJsonValue& v : params["labels"].toArray()) labels.push_back((float)v.toDouble(1));
  if (points.empty()) return errorResult("points is empty");

  auto t0 = std::chrono::steady_clock::now();
  segment::LowResMask lr = sam_->decode_prompt(points, labels);
  segment::DetectedObject o = segment::object_from_logits(*sam_, lr.logits, lr.iou);
  double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
  QJsonValue object = o.area > 0 ? QJsonValue(objectToJson(o, 1)) : QJsonValue();
  return QJsonObject{{"success", true}, {"timeMs", ms}, {"object", object}};
}

void SegmentWorker::handleAction(QWebSocket* socket, const QString& id, const QString& action, const QJsonValue& params) {
  QPointer<QWebSocket> socket_ptr = socket;
  QJsonObject result;
  try {
    if (action == "info") {
      result = info();
    } else if (action == "unload") {
      sam_.reset();
      result = QJsonObject{{"success", true}};
    } else if (action == "detect" || action == "prompt") {
      QString error;
      if (!ensureLoaded(&error)) {
        result = errorResult(error);
      } else {
        // arm before the work so a throwing request still gets the model unloaded
        last_used_ms_ = QDateTime::currentMSecsSinceEpoch();
        QTimer::singleShot(kIdleUnloadMs, this, &SegmentWorker::unloadIfIdle);
        result = action == "detect" ? detect(socket_ptr, id, params.toObject()) : prompt(params.toObject());
        last_used_ms_ = QDateTime::currentMSecsSinceEpoch();  // refresh so a long request is not unloaded right after
      }
    } else {
      result = errorResult("Unknown segment action: " + action);
    }
  } catch (const std::exception& e) {
    qCritical() << "segment: action" << action << "failed:" << e.what();
    result = errorResult(e.what());
  }
  if (socket_ptr.isNull()) return;
  Q_EMIT sendCallbackInMain(socket_ptr.data(), id, result);
}
