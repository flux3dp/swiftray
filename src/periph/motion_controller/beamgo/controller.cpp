#include "controller.h"

#include <QCryptographicHash>
#include <QRegularExpression>
#include <QSettings>
#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

namespace beamgo {
namespace {
uint32_t u32(const Bytes& b) {
  if (b.size() != 4) throw std::invalid_argument("Invalid 32-bit response");
  return (uint32_t(b[0]) << 24) | (uint32_t(b[1]) << 16) | (uint32_t(b[2]) << 8) | b[3];
}
QByteArray array(const Bytes& bytes) { return QByteArray(reinterpret_cast<const char*>(bytes.data()), bytes.size()); }
Bytes bytes(const QByteArray& value) { return Bytes(value.begin(), value.end()); }
bool integer(const QJsonValue& value, int low, int high) {
  return value.isDouble() && std::isfinite(value.toDouble()) && value.toDouble() == std::floor(value.toDouble()) &&
         value.toDouble() >= low && value.toDouble() <= high;
}
QString safetyError(uint32_t error) {
  if (error & 2) return "HARDWARE_ERROR,DOOR_OPENED";
  if (error & 1) return "HARDWARE_ERROR,TILT";
  return error ? "HARDWARE_ERROR,UNKNOWN" : QString();
}
}

Controller::Controller(QString port, QObject* parent, qint32 baudRate)
    : QObject(parent), port_(std::move(port)), baud_rate_(baudRate) {
  poll_.setInterval(500);
  connect(&poll_, &QTimer::timeout, this, &Controller::refresh);
  connect(&transport_, &Transport::disconnected, this, [this] {
    poll_.stop();
    ready_ = false;
    status_ = 128;
    last_error_ = "DISCONNECTED";
    ++generation_;
    if (start_reply_) answer(std::exchange(start_reply_, {}), last_error_);
    if (stop_reply_) answer(std::exchange(stop_reply_, {}), last_error_);
    publish();
    Q_EMIT disconnected();
  });
  connect(&transport_, &Transport::eventReceived, this, [this](const QByteArray& value) {
    if (value.size() < 2) return;
    consume(static_cast<uint8_t>(value[1]), bytes(value.mid(2)));
  });
}

void Controller::answer(Reply reply, const QString& error, QJsonObject data) {
  data["success"] = error.isEmpty();
  if (!error.isEmpty()) data["error"] = error;
  reply(std::move(data), "callback");
}

void Controller::read(const std::vector<int>& parameters, ReadReply reply, Readings readings, size_t index) {
  if (index == parameters.size()) { reply(std::move(readings), {}); return; }
  const int parameter = parameters[index];
  const auto generation = generation_;
  transport_.request({0x11, static_cast<uint8_t>(parameter)},
      [this, parameters, reply = std::move(reply), readings = std::move(readings), index, parameter, generation]
      (const Bytes& message, const QString& error) mutable {
    if (!error.isEmpty()) { reply({}, error); return; }
    if (generation != generation_) { reply({}, "CANCELLED"); return; }
    Bytes body(message.begin() + 2, message.end());
    if (((parameter == 4 || parameter == 6 || parameter == 0x16 || parameter == 0x17) && body.size() != 4) ||
        (parameter == 3 && body.size() != 5) || (parameter == 2 && (body.size() < 24 || body.size() % 2))) {
      reply({}, "INVALID_RESPONSE");
      transport_.close();
      return;
    }
    readings[parameter] = body;
    readings_[parameter] = body;
    consume(parameter, body);
    if (generation != generation_) { reply({}, "CANCELLED"); return; }
    read(parameters, std::move(reply), std::move(readings), index + 1);
  });
}

void Controller::initialize() {
  if (!transport_.open(port_, baud_rate_)) { Q_EMIT disconnected(); return; }
  read({0x0b, 3, 2, 4, 0x16, 0x17}, [this](Readings values, QString error) {
    if (!error.isEmpty()) { transport_.close(); return; }
    const auto fields = QString::fromUtf8(array(values[0x0b]).split('\0').first()).split(';');
    if (fields.size() < 3 || fields[1].isEmpty()) { transport_.close(); return; }
    QString name = fields.value(4).trimmed();
    QString serial = fields[0].trimmed();
    if (serial.size() < 8) serial = name.right(12);
    if (serial.size() < 8) serial = QCryptographicHash::hash(port_.toUtf8(), QCryptographicHash::Sha256).toHex().left(16);
    name.replace(QRegularExpression("HY-LASER", QRegularExpression::CaseInsensitiveOption), "BeamGo");
    if (name.isEmpty()) name = "BeamGo";
    const QString id = QCryptographicHash::hash(serial.toUtf8(), QCryptographicHash::Sha256).toHex();
    identity_ = {{"uuid", "beamgo-" + id}, {"port", "beamgo/" + id}, {"serial", serial},
                 {"hashed_serial", id}, {"name", name}, {"version", fields[1]}, {"model", "fbg1"},
                 {"type", "Galvanometer"}, {"source", "swiftray"}, {"physicalPort", port_}};
    ready_ = true;
    Q_EMIT discovered(identity_);
    publish();
    // Older firmware may not implement this optional capability read. Never probe OTA parameters.
    transport_.request({0x11, 0x1d}, [this](const Bytes& message, const QString& error) {
      if (error.isEmpty() && message.size() == 6) readings_[0x1d] = Bytes(message.begin() + 2, message.end());
      poll_.start();
    }, true);
  });
}

void Controller::refresh() {
  if (!ready_ || polling_ || busy_) return;
  polling_ = true;
  read({4, 6, 0x16, 0x17}, [this](Readings, QString) { polling_ = false; publish(); });
}

void Controller::consume(int parameter, const Bytes& body) {
  if ((parameter == 4 || parameter == 6 || parameter == 0x16 || parameter == 0x17) && body.size() != 4) return;
  if (parameter == 4) {
    state_ = static_cast<int>(u32(body));
    transport_.observeRunning(state_ != 0);
    // Work started on the machine's screen must also reserve this device. Discovery is read-only.
    if (!stopping_ && state_ != 0 && status_ != 1 && status_ != 16 && status_ != 48) {
      external_job_ = true;
      framing_ = state_ == 2;
      status_ = 16;
    } else if (!stopping_ && state_ == 0 && external_job_) {
      external_job_ = false;
      framing_ = false;
      status_ = 0;
    }
    if (state_ == 1 && (status_ == 1 || status_ == 16 || status_ == 48)) seen_run_ = true;
    // Idle is not proof of successful completion; only the 49 45 report completes a job.
  } else if (parameter == 6) {
    processed_ = u32(body);
    if (processed_ == 0xffffffff) processed_ = 0;
  } else if (parameter == 0x16) errors_ = u32(body);
  else if (parameter == 0x17) safety_ = u32(body);
  else if (parameter == 0x45 && body.size() == 13 && !framing_ && seen_run_ &&
           (status_ == 16 || status_ == 48 || status_ == 1)) {
    status_ = body[1] == 1 ? 64 : 128;
    if (status_ == 64) processed_ = total_;
    started_ = false;
  }
  const auto error = safetyError(errors_ & safety_);
  if (ready_ && !stopping_ && !error.isEmpty() && (status_ == 1 || status_ == 16 || status_ == 48)) {
    last_error_ = error;
    stop([](QJsonObject, QString) {});
  }
  publish();
}

QJsonObject Controller::status() const {
  QString error = !last_error_.isEmpty() ? last_error_ : safetyError(errors_ & safety_);
  QJsonValue error_value = error;
  if (error.contains(',')) error_value = QJsonArray::fromStringList(error.split(','));
  return {{"st_id", status_}, {"prog", total_ ? std::min(1.0, double(processed_) / total_) : 0},
          {"error", error_value}, {"disconnection", ready_ ? 0 : 1}};
}

void Controller::publish() {
  if (identity_.isEmpty()) return;
  auto device = identity_;
  device["st_id"] = status_;
  device["st_prog"] = status()["prog"];
  Q_EMIT statusChanged(device);
}

void Controller::guard(bool paused, std::function<void(QString)> callback) {
  read({3, 4, 0x17, 0x16}, [this, paused, callback = std::move(callback)](Readings values, QString error) {
    if (error.isEmpty() && state_ != (paused ? 1 : 0)) error = "JOB_RUNNING";
    if (error.isEmpty() && ((values[3][0] << 8) | values[3][1]) != 90) error = "FIELD_MISMATCH";
    if (error.isEmpty()) error = safetyError(errors_ & safety_);
    callback(error);
  });
}

void Controller::start(Bytes records, bool framing, Reply reply) {
  if (records.empty()) { answer(std::move(reply), "EMPTY_JOB"); return; }
  if (busy_ || stopping_ || status_ == 1 || status_ == 16 || status_ == 48) { answer(std::move(reply), "JOB_RUNNING"); return; }
  const auto generation = ++generation_;
  busy_ = true;
  guard(false, [this, generation, records = std::move(records), framing, reply = std::move(reply)](QString error) mutable {
    busy_ = false;
    if (generation != generation_) { answer(std::move(reply), "CANCELLED"); return; }
    if (!error.isEmpty()) { answer(std::move(reply), error); return; }
    blocks_ = jobBlocks(records, framing);
    total_ = records.size() / 5;
    uploaded_ = next_block_ = processed_ = 0;
    framing_ = framing;
    started_ = seen_run_ = false;
    external_job_ = false;
    last_error_.clear();
    status_ = 1;
    start_reply_ = std::move(reply);
    stream(generation);
  });
}

void Controller::stream(uint64_t generation) {
  if (generation != generation_ || !ready_) return;
  if (next_block_ == blocks_.size()) {
    if (!started_) startMachine(generation);
    return;
  }
  if (!framing_ && started_ && uploaded_ >= size_t(processed_) + 9000) {
    QTimer::singleShot(50, this, [this, generation] { stream(generation); });
    return;
  }
  const size_t index = next_block_++;
  const auto& block = blocks_[index];
  const size_t count = (!framing_ && (index == 0 || index + 1 == blocks_.size())) ? 0 : (block.size() - 4) / 5;
  transport_.request(block, [this, generation, count](const Bytes&, const QString& error) {
    if (generation != generation_) return;
    if (!error.isEmpty()) {
      if (start_reply_) answer(std::exchange(start_reply_, {}), error);
      return;
    }
    uploaded_ += count;
    if (!framing_ && !started_ && uploaded_ >= 3000) startMachine(generation);
    else stream(generation);
  });
}

void Controller::startMachine(uint64_t generation) {
  transport_.request({0x12, static_cast<uint8_t>(framing_ ? 4 : 0), 0},
      [this, generation](const Bytes&, const QString& error) {
    if (generation != generation_) return;
    if (error.isEmpty()) {
      started_ = true;
      if (status_ == 1) status_ = 16;
      publish();
    }
    if (start_reply_) answer(std::exchange(start_reply_, {}), error);
    if (error.isEmpty() && next_block_ < blocks_.size()) stream(generation);
  });
}

void Controller::stop(Reply reply) {
  if (stopping_) { answer(std::move(reply), "JOB_RUNNING"); return; }
  stop_reply_ = std::move(reply);
  // Retain the callback across idle-poll timers so unplugging completes the request immediately.
  reply = [this](QJsonObject result, QString type) {
    if (stop_reply_) std::exchange(stop_reply_, {})(std::move(result), std::move(type));
  };
  ++generation_;
  stopping_ = true;
  status_ = 1;
  started_ = false;
  publish();
  transport_.cancelQueued();
  if (start_reply_) answer(std::exchange(start_reply_, {}), "CANCELLED");
  transport_.request({0x12, 0x05}, [this, reply = std::move(reply)](const Bytes&, const QString& error) mutable {
    if (!error.isEmpty()) { stopping_ = false; answer(std::move(reply), error); return; }
    transport_.request({0x12, 0x01}, [this, reply = std::move(reply)](const Bytes&, const QString& error) mutable {
      framing_ = false;
      if (!error.isEmpty()) { stopping_ = false; answer(std::move(reply), error); return; }
      waitForIdle(std::move(reply));
    }, false, true);
  }, false, true);
}

void Controller::waitForIdle(Reply reply, int attempt) {
  read({4}, [this, reply, attempt](Readings, QString error) {
    // ACK precedes physical idle (about 70 ms in the handoff capture). Keep starts blocked.
    if (error.isEmpty() && state_ != 0 && attempt < 20) {
      QTimer::singleShot(50, this, [this, reply, attempt] { waitForIdle(reply, attempt + 1); });
      return;
    }
    if (error.isEmpty() && state_ != 0) error = "STOP_FAILED";
    stopping_ = false;
    external_job_ = false;
    status_ = error.isEmpty() || !ready_ ? 128 : 16;
    if (!error.isEmpty()) last_error_ = error;
    publish();
    answer(reply, error);
  });
}

void Controller::action(const QString& action, const QJsonObject& params, Reply reply) {
  try {
    if (!ready_) { answer(std::move(reply), "DISCONNECTED"); return; }
    if (action == "getStatus") { answer(std::move(reply), {}, status()); return; }
    if (action == "connect") { answer(std::move(reply)); return; }
    if (action == "checkButton") {
      answer(std::move(reply), {}, {{"pressed", false}, {"isFraming", framing_ && status_ == 16},
                                  {"isRunning", !framing_ && (status_ == 16 || status_ == 48)}});
      return;
    }
    if (action == "stop" || action == "stopFraming") { stop(std::move(reply)); return; }
    if (stopping_) { answer(std::move(reply), "JOB_RUNNING"); return; }
    if (action == "pause") {
      if (framing_ || status_ != 16) { answer(std::move(reply), "INVALID_STATE"); return; }
      transport_.request({0x12, 0x06}, [this, reply](const Bytes&, const QString& error) {
        if (error.isEmpty() && status_ == 16) status_ = 48;
        answer(reply, error);
      }, false, true);
      return;
    }
    if (action == "resume") {
      if (framing_ || status_ != 48) { answer(std::move(reply), "INVALID_STATE"); return; }
      const auto generation = generation_;
      guard(true, [this, generation, reply](QString error) {
        if (generation != generation_) error = "CANCELLED";
        if (!error.isEmpty()) { answer(reply, error); return; }
        transport_.request({0x12, 0x07}, [this, reply](const Bytes&, const QString& error) {
          if (error.isEmpty() && status_ == 48) status_ = 16;
          answer(reply, error);
        }, false, true);
      });
      return;
    }
    if (busy_ || status_ == 1 || status_ == 16 || status_ == 48) { answer(std::move(reply), "JOB_RUNNING"); return; }
    if (action == "upload") {
      records_.clear();
      records_ = compile(params["data"].toString().toStdString());
      answer(std::move(reply), records_.empty() ? "EMPTY_JOB" : QString());
    } else if (action == "start") start(records_, false, std::move(reply));
    else if (action == "startFraming") {
      if ((params.contains("framingPower") && !integer(params["framingPower"], 1, 100)) ||
          !integer(params["width"], 90, 90) || params.contains("rotaryInfo") && !params["rotaryInfo"].isNull()) {
        answer(std::move(reply), "INVALID_PARAMS"); return;
      }
      CompileOptions options;
      options.preview = true;
      options.framing_power = params["framingPower"].toInt(15);
      QString gcode = params["taskCode"].toString();
      if (gcode.isEmpty()) {
        const auto points = params["points"].toArray();
        if (points.size() < 2) { answer(std::move(reply), "INVALID_PARAMS"); return; }
        gcode = "M100\nG90\nM3S0\n";
        for (const auto& point : points) {
          const auto xy = point.toArray();
          if (xy.size() != 2 || !xy[0].isDouble() || !xy[1].isDouble()) { answer(std::move(reply), "INVALID_PARAMS"); return; }
          gcode += QString("G1X%1Y%2\n").arg(xy[0].toDouble(), 0, 'f', 6).arg(xy[1].toDouble(), 0, 'f', 6);
        }
        gcode += "M2\n";
      }
      // Compile before transferring the reply: invalid G-code must still receive an error.
      auto records = compile(gcode.toStdString(), options);
      start(std::move(records), true, reply);
    } else if (action == "quit") {
      status_ = 0;
      last_error_.clear();
      answer(std::move(reply));
    } else if (action == "getDeviceInfo" || action == "info") {
      auto info = identity_;
      const auto& lens = readings_[3];
      info["fieldMm"] = (lens[0] << 8) | lens[1];
      info["featureWord"] = readings_.contains(0x1d) ? double(u32(readings_[0x1d])) : 0;
      answer(std::move(reply), {}, info);
    } else if (action == "getCalibration" || action == "setCalibration" || action == "restoreCalibration") {
      calibration(action, params, std::move(reply));
    } else if (action == "getSafetySettings" || action == "setSafetySettings") {
      safetySettings(action == "getSafetySettings" ? QJsonObject{} : params, std::move(reply));
    } else if (action == "uploadProjectFile") uploadProject(params, std::move(reply));
    else if (action == "getPreview") answer(std::move(reply), {}, {{"time_cost", 0}});
    else answer(std::move(reply), "UNSUPPORTED");
  } catch (const std::exception& error) {
    answer(std::move(reply), QString::fromUtf8(error.what()));
  }
}

void Controller::shutdown() { transport_.close(); }

namespace {
const QStringList kCalibrationNames = {"scaleX", "scaleY", "bucketX", "bucketY",
                                       "paralleX", "paralleY", "trapeX", "trapeY"};
QJsonObject calibrationInfo(const Bytes& raw, const Bytes& lens) {
  QJsonObject info{{"fieldMm", (lens[0] << 8) | lens[1]}, {"swapXY", bool(lens[2])},
                   {"reverseX", bool(lens[3])}, {"reverseY", bool(lens[4])},
                   {"rawHex", QString::fromLatin1(array(raw).toHex())}, {"length", int(raw.size())}};
  for (int i = 0; i < 8; ++i) {
    const int16_t value = static_cast<int16_t>((raw[2 * i] << 8) | raw[2 * i + 1]);
    info[kCalibrationNames[i]] = value / 10000.0 + (i < 2 ? 0 : 1);
  }
  return info;
}
}

void Controller::writeVerified(int parameter, const Bytes& value, Response callback) {
  const auto generation = generation_;
  Bytes request{0x10, static_cast<uint8_t>(parameter)};
  request.insert(request.end(), value.begin(), value.end());
  transport_.request(std::move(request), [this, parameter, value, callback, generation](const Bytes&, const QString& error) {
    if (!error.isEmpty()) { callback({}, error); return; }
    if (generation != generation_) { callback({}, "CANCELLED"); return; }
    read({parameter}, [parameter, value, callback](Readings readings, QString error) {
      const auto actual = readings.value(parameter);
      if (error.isEmpty() && actual != value) error = "READBACK_MISMATCH";
      callback(actual, error);
    });
  });
}

void Controller::calibration(const QString& action, const QJsonObject& params, Reply reply) {
  busy_ = true;
  read({4, 0x17, 0x16, 3, 2}, [this, action, params, reply](Readings values, QString error) {
    if (!error.isEmpty() || state_ != 0 || errors_ & safety_) {
      busy_ = false;
      answer(reply, !error.isEmpty() ? error : state_ != 0 ? "JOB_RUNNING" : safetyError(errors_ & safety_));
      return;
    }
    const Bytes before = values[2], before_lens = values[3];
    if (action == "getCalibration") {
      busy_ = false;
      answer(reply, {}, calibrationInfo(before, before_lens));
      return;
    }
    Bytes updated = before, lens = before_lens;
    const auto lens_params = action == "restoreCalibration" ? params["lens"].toObject() : params;
    if (lens_params.contains("fieldMm")) {
      if (!integer(lens_params["fieldMm"], 10, 600)) error = "INVALID_PARAMS";
      else { lens[0] = lens_params["fieldMm"].toInt() >> 8; lens[1] = lens_params["fieldMm"].toInt() & 0xff; }
    }
    const QStringList flags{"swapXY", "reverseX", "reverseY"};
    for (int i = 0; i < flags.size(); ++i) {
      if (!lens_params.contains(flags[i])) continue;
      if (!lens_params[flags[i]].isBool()) error = "INVALID_PARAMS";
      else lens[2 + i] = lens_params[flags[i]].toBool();
    }
    if (action == "restoreCalibration") {
      const auto hex = params["rawHex"].toString();
      if (!params["lens"].isObject() || hex.size() != int(before.size() * 2) ||
          !QRegularExpression("^[0-9a-fA-F]+$").match(hex).hasMatch()) error = "INVALID_PARAMS";
      else updated = bytes(QByteArray::fromHex(hex.toLatin1()));
    } else {
      if (lens[2] != before_lens[2]) {
        for (int i = 0; i < 16; i += 4) {
          std::swap(updated[i], updated[i + 2]);
          std::swap(updated[i + 1], updated[i + 3]);
        }
      }
      for (int i = 0; i < 8; ++i) {
        const auto key = kCalibrationNames[i];
        if (!params.contains(key)) continue;
        const auto value = params[key];
        const double low = i < 2 ? 0.1 : 0.5, high = i < 2 ? 1 : 1.5;
        if (!value.isDouble() || !std::isfinite(value.toDouble()) || value.toDouble() < low || value.toDouble() > high) {
          error = "INVALID_PARAMS";
          break;
        }
        const auto word = static_cast<uint16_t>(std::lround((value.toDouble() - (i < 2 ? 0 : 1)) * 10000));
        updated[2 * i] = word >> 8;
        updated[2 * i + 1] = word & 0xff;
      }
    }
    if (!error.isEmpty()) { busy_ = false; answer(reply, error); return; }
    // Save the complete original data before either register is written, including lens flags.
    QSettings backup;
    const QString key = "beamgo/calibration-backup/" + identity_["hashed_serial"].toString();
    backup.setValue(key + "/rawHex", array(before).toHex());
    backup.setValue(key + "/lensHex", array(before_lens).toHex());
    backup.sync();
    if (backup.status() != QSettings::NoError) { busy_ = false; answer(reply, "BACKUP_FAILED"); return; }
    const auto finish = [this, reply, before, before_lens](QString error) {
      busy_ = false;
      answer(reply, error, {{"backupHex", QString::fromLatin1(array(before).toHex())},
                           {"backupLens", calibrationInfo(before, before_lens)},
                           {"readback", calibrationInfo(readings_[2], readings_[3])}});
    };
    const auto write_calibration = [this, updated, finish] {
      writeVerified(2, updated, [finish](const Bytes&, const QString& error) { finish(error); });
    };
    if (lens == before_lens) write_calibration();
    else writeVerified(3, lens, [write_calibration, finish](const Bytes&, const QString& error) {
      if (!error.isEmpty()) finish(error);
      else write_calibration();
    });
  });
}

void Controller::safetySettings(const QJsonObject& params, Reply reply) {
  for (const auto& name : {"doorCheck", "tiltCheck"}) {
    if (params.contains(name) && !params[name].isBool()) { answer(reply, "INVALID_PARAMS"); return; }
  }
  busy_ = true;
  read({4, 0x17, 0x16}, [this, params, reply](Readings values, QString error) {
    if (!error.isEmpty()) { busy_ = false; answer(reply, error); return; }
    const auto finish = [this, reply](QString error) {
      busy_ = false;
      answer(reply, error, {{"doorCheck", bool(safety_ & 2)}, {"tiltCheck", bool(safety_ & 1)}, {"mask", double(safety_)}});
    };
    if (params.isEmpty()) { finish({}); return; }
    if (state_ != 0 || errors_ & safety_) {
      finish(state_ != 0 ? "JOB_RUNNING" : safetyError(errors_ & safety_));
      return;
    }
    uint32_t mask = safety_;
    if (params.contains("doorCheck")) mask = (mask & ~2u) | (params["doorCheck"].toBool() ? 2u : 0u);
    if (params.contains("tiltCheck")) mask = (mask & ~1u) | (params["tiltCheck"].toBool() ? 1u : 0u);
    const auto encoded = record32(0, mask);
    writeVerified(0x17, Bytes(encoded.begin() + 1, encoded.end()),
                  [finish](const Bytes&, const QString& error) { finish(error); });
  });
}

void Controller::uploadProject(const QJsonObject& params, Reply reply) {
  const auto encoded = params["dataBase64"].toString().toLatin1();
  if (encoded.size() > 48 * 1024 * 1024) { answer(reply, "FILE_TOO_LARGE"); return; }
  const auto original = QByteArray::fromBase64(encoded, QByteArray::AbortOnBase64DecodingErrors);
  if (original.isEmpty()) { answer(reply, "INVALID_PARAMS"); return; }
  const auto compressed = qCompress(original);
  auto packets = std::make_shared<std::vector<Bytes>>();
  const int count = (compressed.size() + 4999) / 5000;
  for (int i = 0; i <= count; ++i) {
    const auto payload = i == count ? QByteArray{} : compressed.mid(i * 5000, 5000);
    const int sequence = i == count ? 0xffff : i;
    QByteArray body;
    body.append(char(sequence >> 8)); body.append(char(sequence & 0xff));
    body.append(char(payload.size() >> 8)); body.append(char(payload.size() & 0xff));
    body.append(payload);
    QByteArray packet = QByteArray::fromHex("1021") + body + QCryptographicHash::hash(body, QCryptographicHash::Md5);
    packets->push_back(bytes(packet));
  }
  busy_ = true;
  const auto generation = ++generation_;
  guard(false, [this, generation, packets, reply](QString error) {
    if (generation != generation_) error = "CANCELLED";
    if (!error.isEmpty()) { busy_ = false; answer(reply, error); return; }
    sendFilePackets(packets, 0, generation, reply);
  });
}

void Controller::sendFilePackets(std::shared_ptr<std::vector<Bytes>> packets, size_t index,
                                  uint64_t generation, Reply reply) {
  if (generation != generation_) { busy_ = false; answer(reply, "CANCELLED"); return; }
  if (index == packets->size()) { busy_ = false; answer(reply); return; }
  transport_.request(packets->at(index), [this, packets, index, generation, reply](const Bytes&, const QString& error) {
    if (!error.isEmpty()) { busy_ = false; answer(reply, error); return; }
    reply({{"percentage", double(index + 1) / packets->size() * 100}}, "progress");
    sendFilePackets(packets, index + 1, generation, reply);
  });
}
}  // namespace beamgo
