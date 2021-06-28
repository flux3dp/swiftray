#include "preset-settings.h"

PresetSettings::Param PresetSettings::Param::fromJson(const QJsonObject &obj) {
  Param p;
  p.name = obj["name"].toString();
  p.power = obj["power"].toInt();
  p.speed = obj["speed"].toInt();
  p.repeat = obj["repeat"].toInt(1);
  p.step_height = obj["step_height"].toDouble();
  p.target_height = obj["target_height"].toDouble();
  p.use_diode = obj["isUseDiode"].toBool();
  p.use_autofocus = obj["useAutofocus"].toBool();
  return p;
}

QJsonObject PresetSettings::Param::toJson() const {
  QJsonObject obj;
  obj["name"] = name;
  obj["power"] = power;
  obj["speed"] = speed;
  obj["repeat"] = repeat;
  obj["step_height"] = step_height;
  obj["target_height"] = target_height;
  obj["isUseDiode"] = use_diode;
  obj["useAutofocus"] = use_autofocus;
  return obj;
}


PresetSettings::Preset PresetSettings::Preset::fromJson(const QJsonObject &obj) {
  PresetSettings::Preset p;
  QJsonArray data = obj["data"].toArray();
  for (QJsonValue item : data) {
    p.params << PresetSettings::Param::fromJson(item.toObject());
  }
  p.name = obj["name"].toString();
  return p;
}

QJsonObject PresetSettings::Preset::toJson() const {
  QJsonArray data;
  for (auto &param : params) {
    data << param.toJson();
  }
  QJsonObject obj;
  obj["name"] = name;
  obj["data"] = data;
  return obj;
}

