#include "param-settings.h"

ParamSettings::ParamSet ParamSettings::ParamSet::fromJson(const QJsonObject &obj) {
  ParamSet p;
  p.name = obj["name"].toString();
  p.power = obj["power"].toInt();
  p.speed = obj["speed"].toInt();
  p.repeat = obj["repeat"].toInt(1);
  p.step_height = obj["step_height"].toDouble();
  p.target_height = obj["target_height"].toDouble();
  p.is_diode = obj["isUseDiode"].toBool();
  p.use_autofocus = obj["useAutofocus"].toBool();
  return p;
}

QJsonObject ParamSettings::ParamSet::toJson() const {
  QJsonObject obj;
  obj["name"] = name;
  obj["power"] = power;
  obj["speed"] = speed;
  obj["repeat"] = repeat;
  obj["step_height"] = step_height;
  obj["target_height"] = target_height;
  obj["isUseDiode"] = is_diode;
  obj["useAutofocus"] = use_autofocus;
  return obj;
}