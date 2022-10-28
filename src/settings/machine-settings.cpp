#include <windows/osxwindow.h>
#include "machine-settings.h"

typedef MachineSettings::MachineSet MachineSet;

QList<MachineSet> MachineSettings::machineDatabase_;

MachineSettings::MachineSettings() {
  QSettings settings;
  QJsonObject obj = settings.value("machines/machines").value<QJsonDocument>().object();
  loadJson(obj);
}

/**
 * @brief Load multiple machine sets from JSON format
 *        Not used currently
 * 
 * @param obj 
 */
void MachineSettings::loadJson(const QJsonObject &obj) {
  if (obj["data"].isNull()) {
    qWarning() << "[MachineSettings] Cannot load machine settings";
    return;
  }
  QJsonArray data = obj["data"].toArray();
  machines_mutex_.lock();
  machines_.clear();
  for (QJsonValue item : data) {
    MachineSet machine = MachineSet::fromJson(item.toObject());
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

const QList<MachineSet> &MachineSettings::machines() {
  return machines_;
}

void MachineSettings::addMachine(MachineSet mach) {
  machines_ << mach;
}

void MachineSettings::clearMachines() {
  machines_.clear();
}

void MachineSettings::save() {
  QSettings settings;
  settings.setValue("machines/machines", QJsonDocument(toJson()));
}

QIcon MachineSet::icon() const {
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

MachineSet MachineSet::fromJson(const QJsonObject &obj) {
  MachineSet m;

  m.id = obj["id"].toString();
  m.name = obj["name"].toString();
  m.brand = obj["brand"].toString();
  m.model = obj["model"].toString();

  m.icon_url = obj["icon"].toString();

  m.width = obj["width"].toInt();
  m.height = obj["height"].toInt();

  m.origin = (MachineSet::OriginType) obj["origin"].toInt();
  m.home_on_start = obj["homeOnStart"].toBool();

  m.board_type = (MachineSet::BoardType) obj["boardType"].toInt();
  m.red_pointer_offset = QPointF(
       obj["redPointerOffsetX"].toInt(),
       obj["redPointerOffsetY"].toInt()
  );
  return m;
}

QJsonObject MachineSet::toJson() const {
  QJsonObject obj;
  obj["id"] = id;
  obj["name"] = name;
  obj["model"] = model;
  obj["width"] = width;
  obj["height"] = height;
  obj["origin"] = (int) origin;
  obj["icon"] = icon_url;
  obj["homeOnStart"] = home_on_start;
  obj["boardType"] = (int) board_type;
  obj["redPointerOffsetX"] = red_pointer_offset.x();
  obj["redPointerOffsetY"] = red_pointer_offset.y();
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
QList<MachineSet> MachineSettings::database() {
  if (machineDatabase_.empty()) {
    QFile file(":/resources/machines.json");
    file.open(QFile::ReadOnly);
    auto data = QJsonDocument::fromJson(file.readAll()).object()["data"].toArray();
    for (QJsonValue item : data) {
      MachineSettings::machineDatabase_ << MachineSet::fromJson(item.toObject());
    }
  }
  return MachineSettings::machineDatabase_;
}

/**
 * @brief Try to find specific brand and model from preset (predefined) machines
 * 
 * @param brand 
 * @param model 
 * @return MachineSet 
 */
MachineSet MachineSettings::findPreset(QString brand, QString model) {
  for (MachineSet m : MachineSettings::database()) {
    if (m.brand == brand && m.model == model) {
      return m;
    }
  }
  return MachineSet();
}

/**
 * @brief The brands in the preset (predefined) machines
 * 
 * @return QStringList 
 */
QStringList MachineSettings::brands() {
  QList<QString> result;
  for (MachineSet m : MachineSettings::database()) {
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
  for (MachineSet m : MachineSettings::database()) {
    if (m.brand == brand) {
      result << m.model;
    }
  }
  result << tr("Other");
  return QStringList(result);
}