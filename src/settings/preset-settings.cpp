#include "preset-settings.h"
#include <QJsonArray>

PresetSettings::PresetSettings() {
}

PresetSettings::Param PresetSettings::Param::fromJson(const QJsonObject &obj) {
  Param p;
  p.name = obj["name"].toString();
  p.power = obj["power"].toDouble();
  p.speed = obj["speed"].toDouble();
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
  p.framing_power = obj["framing_power"].toDouble();
  p.pulse_power = obj["pulse_power"].toDouble();
  return p;
}

QJsonObject PresetSettings::Preset::toJson() const {
  QJsonArray data;
  for (auto &param : params) {
    data << param.toJson();
  }
  QJsonObject obj;
  obj["name"] = name;
  obj["framing_power"] = framing_power;
  obj["pulse_power"] = pulse_power;
  obj["data"] = data;
  return obj;
}

void PresetSettings::setPresets(const QList<Preset> &new_presets) {
  presets_.clear();
  for (auto &preset : new_presets) {
    presets_ << preset;
  }
}

void PresetSettings::setOriginPresets(const QList<Preset> &presets) {
  origin_preset_.clear();
  for (auto &preset : presets) {
    origin_preset_ << preset;
  }
}

void PresetSettings::save() {
  Q_EMIT savePreset(toJson());
}

void PresetSettings::reset() {
  presets_.clear();
  for (auto &preset : origin_preset_) {
    presets_ << preset;
  }
  Q_EMIT resetPreset();
}

void PresetSettings::loadPreset(const QJsonObject &obj) {
  QJsonArray data = obj["data"].toArray();
  presets_.clear();
  for (QJsonValue item : data) {
    presets_ << Preset::fromJson(item.toObject());
  }
}

QJsonObject PresetSettings::toJson() const {
  QJsonArray data;
  for (auto &preset : presets_) {
    data << preset.toJson();
  }
  QJsonObject obj;
  obj["data"] = data;
  return obj;
}

PresetSettings::Preset PresetSettings::getTargetPreset(int preset_index) {
  if(preset_index < presets_.size()) {
    return presets_[preset_index];
  } else {
    return Preset();
  }
}

PresetSettings::Param PresetSettings::getTargetParam(int preset_index, int param_index) {
  Preset tmp_preset = getTargetPreset(preset_index);
  if(!tmp_preset.params.empty() && param_index < tmp_preset.params.size()) {
    return tmp_preset.params[param_index];
  } else {
    return Param();
  }
}

void PresetSettings::setPresetPower(int preset_index, double framing, double pulse) {
  presets_[preset_index].framing_power = framing;
  presets_[preset_index].pulse_power = pulse;
}