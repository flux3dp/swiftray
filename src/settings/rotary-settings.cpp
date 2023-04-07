#include "rotary-settings.h"
#include <QJsonArray>

RotarySettings::RotarySettings() {
}

RotarySettings::RotaryParam RotarySettings::RotaryParam::fromJson(const QJsonObject &obj) {
  RotaryParam p;
  p.name = obj["name"].toString();
  p.travel_speed = obj["travel_speed"].toDouble();
  p.rotary_type = (RotaryType) obj["rotary_type"].toInt();
  p.mm_per_rotation = obj["mm_per_rotation"].toDouble();
  p.roller_diameter = obj["roller_diameter"].toDouble();
  return p;
}

QJsonObject RotarySettings::RotaryParam::toJson() const {
  QJsonObject obj;
  obj["name"] = name;
  obj["travel_speed"] = travel_speed;
  obj["rotary_type"] = (int) rotary_type;
  obj["mm_per_rotation"] = mm_per_rotation;
  obj["roller_diameter"] = roller_diameter;
  return obj;
}

void RotarySettings::setRotarys(const QList<RotaryParam> &new_rotarys) {
  rotarys_.clear();
  for (auto &rotary : new_rotarys) {
    rotarys_ << rotary;
  }
}

void RotarySettings::addRotary(RotaryParam new_rotary) {
  rotarys_ << new_rotary;
}

void RotarySettings::save() {
  Q_EMIT saveRotary(toJson());
}

void RotarySettings::loadRotary(const QJsonObject &obj) {
  QJsonArray data = obj["data"].toArray();
  rotarys_.clear();
  for (QJsonValue item : data) {
    rotarys_ << RotaryParam::fromJson(item.toObject());
  }
}

QJsonObject RotarySettings::toJson() const {
  QJsonArray data;
  for (auto &rotary : rotarys_) {
    data << rotary.toJson();
  }
  QJsonObject obj;
  obj["data"] = data;
  return obj;
}

RotarySettings::RotaryParam RotarySettings::getTargetRotary(int rotary_index) {
  if(rotary_index < rotarys_.size()) {
    return rotarys_[rotary_index];
  } else {
    return RotaryParam();
  }
}

void RotarySettings::setRotarySpeed(int rotary_index, double travel_speed) {
  if(travel_speed > 0 && rotary_index < rotarys_.size()) {
    rotarys_[rotary_index].travel_speed = travel_speed;
  }
}