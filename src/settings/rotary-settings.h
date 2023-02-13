#pragma once

#include <QList>
#include <QString>
#include <QJsonObject>

class RotarySettings : public QObject {
Q_OBJECT
public:
  enum RotaryType{
    Roller = 0,
    Chuck
  };
  static RotarySettings& getInstance() {
    static RotarySettings sInstance;
    return sInstance;
  }
  struct RotaryParam {
    QString name;
    double travel_speed;
    RotaryType rotary_type;
    double mm_per_rotation;
	  double roller_diameter;//mm

    RotaryParam() :
      name("New Rotary"),
      travel_speed(5),
      rotary_type(Roller),
      mm_per_rotation(55),
      roller_diameter(26) {}

    static RotaryParam fromJson(const QJsonObject &obj);
    QJsonObject toJson() const;
  };

  const QList<RotaryParam> &rotarys() {
    return rotarys_;
  }
  void setRotarys(const QList<RotaryParam> &new_rotarys);
  void addRotary(RotaryParam new_rotary);
  void save();
  void loadRotary(const QJsonObject &obj);
  QJsonObject toJson() const;
  RotaryParam getTargetRotary(int rotary_index);
  void setRotarySpeed(int rotary_index, double travel_speed);

Q_SIGNALS:
  void saveRotary(QJsonObject save_obj);

private:
  RotarySettings();

  QList<RotaryParam> rotarys_;
};