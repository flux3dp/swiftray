#include <windows/osxwindow.h>
#include "machine-settings.h"

typedef MachineSettings::MachineParam MachineParam;

QList<MachineParam> MachineSettings::machineDatabase_;

MachineSettings::MachineSettings() {
}

/**
 * @brief Load multiple machine sets from JSON format
 *        Not used currently
 * 
 * @param obj 
 */
void MachineSettings::loadJson(const QJsonObject &obj) {
  QJsonArray data = obj["data"].toArray();
  machines_mutex_.lock();
  machines_.clear();
  for (QJsonValue item : data) {
    MachineParam machine = MachineParam::fromJson(item.toObject());
    machines_ << machine;
  }
  machines_mutex_.unlock();
}

/**
 * @brief Convert the machine list into JSON object
 *        Not used currently
 * 
 * @return QJsonObject 
 */
QJsonObject MachineSettings::toJson() {
  QJsonArray data;
  machines_mutex_.lock();
  for (auto &mach : machines_) {
    data << mach.toJson();
  }
  machines_mutex_.unlock();
  QJsonObject obj;
  obj["data"] = data;
  return obj;
}

const QList<MachineParam> &MachineSettings::getMachines() {
  return machines_;
}

void MachineSettings::setMachines(const QList<MachineParam> &new_machines) {
  machines_mutex_.lock();
  machines_.clear();
  for (auto &mach : new_machines) {
    machines_ << mach;
  }
  machines_mutex_.unlock();
}

void MachineSettings::addMachine(MachineParam mach) {
  machines_mutex_.lock();
  machines_ << mach;
  machines_mutex_.unlock();
}

void MachineSettings::clearMachines() {
  machines_.clear();
}

void MachineSettings::save() {
  Q_EMIT saveMachines(toJson());
}

QIcon MachineParam::icon() const {
  if (!isDarkMode()) return QIcon(icon_url);
  auto img = QImage(icon_url).convertToFormat(QImage::Format::Format_ARGB32);
  for (int y = 0; y < img.height(); ++y) {
    for (int x = 0; x < img.width(); ++x) {
      QRgb color = img.pixel(x, y);
      color = qRgba(255, 255, 255, qAlpha(color));
      img.setPixel(x, y, color);
    }
  }

  return QIcon(QPixmap::fromImage(img));
}

MachineParam MachineParam::fromJson(const QJsonObject &obj) {
  MachineParam m;

  m.id = obj["id"].toString();
  m.name = obj["name"].toString();
  m.brand = obj["brand"].toString();
  m.model = obj["model"].toString();

  m.icon_url = obj["icon"].toString();

  m.width = obj["width"].toInt();
  if(m.width <= 0) m.width = 5;
  m.height = obj["height"].toInt();

  m.origin = (MachineParam::OriginType) obj["origin"].toInt();
  m.home_on_start = obj["homeOnStart"].toBool();

  m.board_type = (MachineParam::BoardType) obj["boardType"].toInt();
  m.red_pointer_offset = QPointF(
       obj["redPointerOffsetX"].toInt(),
       obj["redPointerOffsetY"].toInt()
  );
  if(!obj["travel_speed"].isNull()) m.travel_speed = obj["travel_speed"].toDouble();
  else {
    m.travel_speed = 100;
  }
  if(!obj["rotary_axis"].isNull()) m.rotary_axis = *obj["rotary_axis"].toString().toStdString().c_str();
  else {
    m.rotary_axis = 'Y';
  }
  if(!obj["is_high_speed_mode"].isNull()) m.is_high_speed_mode = obj["is_high_speed_mode"].toBool();
  else {
    std::size_t found = m.name.toStdString().find("Lazervida");
    if(found!=std::string::npos) {
      m.is_high_speed_mode = true;
    } else {
      m.is_high_speed_mode = false;
    }
  }
  return m;
}

QJsonObject MachineParam::toJson() const {
  QJsonObject obj;
  obj["id"] = id;
  obj["name"] = name;
  obj["model"] = model;
  obj["width"] = width;
  obj["height"] = height;
  obj["origin"] = (int) origin;
  obj["icon"] = icon_url;
  obj["travel_speed"] = travel_speed;
  obj["rotary_axis"] = (QString) rotary_axis;
  obj["homeOnStart"] = home_on_start;
  obj["boardType"] = (int) board_type;
  obj["redPointerOffsetX"] = red_pointer_offset.x();
  obj["redPointerOffsetY"] = red_pointer_offset.y();
  obj["is_high_speed_mode"] = is_high_speed_mode;
  return obj;
}

/**
 * @brief Lazy load and return preset (predefined) machines
 *        Allow newbie user to select without knowing details of machine
 * 
 *        NOTE: The predefined machines aren't listed in machines_, 
 *              they only help user to create and add machine to machines_
 * 
 * @return QList<MachineSet> 
 */
QList<MachineParam> MachineSettings::database() {
  if (machineDatabase_.empty()) {
    QFile file(":/resources/machines.json");
    file.open(QFile::ReadOnly);
    auto data = QJsonDocument::fromJson(file.readAll()).object()["data"].toArray();
    for (QJsonValue item : data) {
      MachineSettings::machineDatabase_ << MachineParam::fromJson(item.toObject());
    }
  }
  return MachineSettings::machineDatabase_;
}

/**
 * @brief Try to find specific brand and model from preset (predefined) machines
 * 
 * @param brand 
 * @param model 
 * @return MachineParam 
 */
MachineParam MachineSettings::findPreset(QString brand, QString model) {
  for (MachineParam m : MachineSettings::database()) {
    if (m.brand == brand && m.model == model) {
      return m;
    }
  }
  qInfo() << Q_FUNC_INFO << __LINE__;
  return MachineParam();
}

MachineParam MachineSettings::getTargetMachine(int machine_index) {
  if(machine_index < machines_.size()) {
    return machines_[machine_index];
  } else {
    return MachineParam();
  }
}

/**
 * @brief The brands in the preset (predefined) machines
 * 
 * @return QStringList 
 */
QStringList MachineSettings::brands() {
  QList<QString> result;
  for (MachineParam m : MachineSettings::database()) {
    if (!result.contains(m.brand)) {
      result << m.brand;
    }
  }
  result << tr("Other");
  return QStringList(result);
}

/**
 * @brief The models in the preset (predefined) machines
 * 
 * @return QStringList 
 */
QStringList MachineSettings::models(QString brand) {
  QList<QString> result;
  for (MachineParam m : MachineSettings::database()) {
    if (m.brand == brand) {
      result << m.model;
    }
  }
  result << tr("Other");
  return QStringList(result);
}

void MachineSettings::setMachineRange(int machine_index, int width, int height) {
  if(width > 0) machines_[machine_index].width = width;
  machines_[machine_index].height = height;
}

void MachineSettings::setMachineTravelSpeed(int machine_index, double speed) {
  if(speed > 0) machines_[machine_index].travel_speed = speed;
}

void MachineSettings::setMachineRotaryAxis(int machine_index, char axis) {
  machines_[machine_index].rotary_axis = axis;
}

void MachineSettings::setMachineHightSpeedMode(int machine_index, bool is_hight_mode) {
  machines_[machine_index].is_high_speed_mode = is_hight_mode;
}

void MachineSettings::setMachineStartWithHome(int machine_index, bool start_with_home) {
  machines_[machine_index].home_on_start = start_with_home;
}