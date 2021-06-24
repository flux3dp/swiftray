//
// Created by Simon on 2021/6/24.
//

#ifndef PARAM_SETTINGS_H
#define PARAM_SETTINGS_H

#include <QString>
#include <QFile>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>
#include <QJsonDocument>


class ParamSettings {
public:
  struct ParamSet {
  public:
    QString name;
    int power;
    int speed;
    int repeat;
    double step_height;
    double target_height;
    bool is_diode;
    bool use_autofocus;

    static ParamSet fromJson(const QJsonObject &obj);

    QJsonObject toJson();
  };

  ParamSettings() {
    QSettings settings;
    QJsonObject obj = settings.value("parameters/user").value<QJsonDocument>().object();
    if (obj["data"].isNull()) {
      QFile file(":/resources/beamo.json");
      file.open(QFile::ReadOnly);
      loadJson(QJsonDocument::fromJson(file.readAll()).object());
    } else {
      loadJson(obj);
    }
  }

  void loadJson(const QJsonObject &obj) {
    QJsonArray data = obj["data"].toArray();
    params.clear();
    for (QJsonValue item : data) {
      ParamSet param = ParamSet::fromJson(item.toObject());
      params << param;
    }
  }

  QJsonObject toJson() {
    QJsonArray data;
    for (auto &param : params) {
      data << param.toJson();
    }
    QJsonObject obj;
    obj["data"] = data;
    return obj;
  }

  QList<ParamSet> params;
};

#endif //PARAM_SETTINGS_H
