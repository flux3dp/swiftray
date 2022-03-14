#pragma once

#include <QString>
#include <QFile>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>
#include <QJsonDocument>

// TODO (Redesign logic to PresetSettings -> Preset -> Param)

class PresetSettings {
public:
  static PresetSettings& getInstance() {
    static PresetSettings sInstance;
    return sInstance;
  }
  struct Param {
  public:
    QString name;
    int power;
    int speed;
    int repeat;
    double step_height;
    double target_height;
    bool use_diode;
    bool use_autofocus;

    Param() :
         power(20),
         speed(20),
         repeat(1),
         step_height(0),
         target_height(0),
         use_diode(false),
         use_autofocus(false) {}

    static Param fromJson(const QJsonObject &obj);

    QJsonObject toJson() const;
  };

  struct Preset {
  public:
    QString name;
    QList<Param> params;

    static Preset fromJson(const QJsonObject &obj);

    QJsonObject toJson() const;
  };

  void loadJson(const QJsonObject &obj) {
    QJsonArray data = obj["data"].toArray();
    presets_.clear();
    for (QJsonValue item : data) {
      presets_ << Preset::fromJson(item.toObject());
    }
  }

  QJsonObject toJson() const {
    QJsonArray data;
    for (auto &param : presets_) {
      data << param.toJson();
    }
    QJsonObject obj;
    obj["data"] = data;
    return obj;
  }

  const QList<Preset> &presets() {
    return presets_;
  }

  void setCurrentIndex(int index) {
    current_index_ = index;
  }

  const Preset currentPreset() {
    qInfo() << "Current Preset:" << current_index_ ;
    return presets_[current_index_];
  }

  void save() {
    QSettings settings;
    settings.setValue("preset/user", QJsonDocument(toJson()));
  }

  QList<Preset> presets_;
  int current_index_ = 0;

private:
  PresetSettings() {
    //QSettings settings;
    //QJsonObject obj = settings.value("preset/user").value<QJsonDocument>().object();
    //if (obj["data"].isNull()) {
    QList<QString> file_list;
    //file_list.append("default.json");
    file_list.append("1.6W.json");
    file_list.append("5W.json");
    file_list.append("10W.json");
    for (int i = 0; i < file_list.size(); ++i) {
      QFile file(":/resources/parameters/"+file_list[i]);
      file.open(QFile::ReadOnly);
      // TODO (Is it possible to remove QJsonDocument and use QJsonObject only?)
      auto preset = Preset::fromJson(QJsonDocument::fromJson(file.readAll()).object());
      qInfo() << file.fileName();
      preset.name = file_list[i].left(file_list[i].lastIndexOf('.'));
      presets_ << preset;
    }
    //} else {
    //  loadJson(obj);
    //}
  }
};
