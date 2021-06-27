#include "machine-settings.h"

MachineSettings::MachineSet MachineSettings::MachineSet::fromJson(const QJsonObject &obj) {
  MachineSet m;

  m.id = obj["id"].toString();
  m.name = obj["name"].toString();
  m.model = obj["model"].toString();

  m.icon = obj["icon"].toString();

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

QJsonObject MachineSettings::MachineSet::toJson() const {
  QJsonObject obj;
  obj["id"] = id;
  obj["name"] = name;
  obj["model"] = model;
  obj["width"] = width;
  obj["height"] = height;
  obj["origin"] = (int) origin;
  obj["icon"] = icon;
  obj["homeOnStart"] = home_on_start;
  obj["boardType"] = (int) board_type;
  obj["redPointerOffsetX"] = red_pointer_offset.x();
  obj["redPointerOffsetY"] = red_pointer_offset.y();
  return obj;
}