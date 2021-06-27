#ifndef MACH_SETTINGS_H
#define MACH_SETTINGS_H

#include <QString>
#include <QFile>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>
#include <QJsonDocument>
#include <QPointF>
#include <QIcon>

class MachineSettings {
public:
  struct MachineSet {
    enum class OriginType {
      TopLeft,
      TopRight,
      BottomLeft,
      BottomRight
    };
    enum class BoardType {
      FLUX_2020,
      GRBL_2020,
      M2NANO_7,
      RUIDA_2020
    };
  public:
    QString id;
    QString name;
    QString model;
    QString icon;

    BoardType board_type;
    OriginType origin;
    int width;
    int height;
    bool home_on_start;
    QPointF red_pointer_offset;


    // Functions
    static MachineSet fromJson(const QJsonObject &obj);

    QJsonObject toJson() const;
  };

  MachineSettings() {
    QSettings settings;
    QJsonObject obj = settings.value("machines").value<QJsonDocument>().object();
    loadJson(obj);
  }

  void loadJson(const QJsonObject &obj) {
    if (obj["data"].isNull()) {
      qWarning() << "[MachineSettings] Cannot load machine settings";
      return;
    }
    QJsonArray data = obj["data"].toArray();
    machines_.clear();
    for (QJsonValue item : data) {
      MachineSet machine = MachineSet::fromJson(item.toObject());
      machines_ << machine;
    }
  }

  QJsonObject toJson() {
    QJsonArray data;
    for (auto &mach : machines_) {
      data << mach.toJson();
    }
    QJsonObject obj;
    obj["data"] = data;
    return obj;
  }

  const QList<MachineSet> &machines() {
    return machines_;
  }

  void save() {
    QSettings settings;
    settings.setValue("machines", toJson());
  }

  QList<MachineSet> machines_;
};

#endif //MACH_SETTINGS_H
