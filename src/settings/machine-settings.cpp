#include "machine-settings.h"

MachineSettings::MachineSet MachineSettings::MachineSet::fromJson(const QJsonObject &obj) {
  MachineSet m;

  m.name = obj["name"].toString();
  m.model_id = obj["modelId"].toString();

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
  obj["name"] = name;
  obj["modelId"] = model_id;
  obj["width"] = width;
  obj["height"] = height;
  obj["origin"] = (int) origin;
  obj["homeOnStart"] = home_on_start;
  obj["boardType"] = (int) board_type;
  obj["redPointerOffsetX"] = red_pointer_offset.x();
  obj["redPointerOffsetY"] = red_pointer_offset.y();
  return obj;
}