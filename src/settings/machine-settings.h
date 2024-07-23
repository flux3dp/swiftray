#pragma once

#include <QString>
#include <QFile>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QPointF>
#include <QIcon>
#include <QStringList>
#include <QMutex>


/**
 *  \class MachineSettings
 *  \brief Manage machine settings
 *
 *  MachineSettings automatically load app settings when constructed.
*/
class MachineSettings : public QObject {
Q_OBJECT
public:
  MachineSettings();
  /**
      \class MachineParam
      \brief The MachineParam class represents a machine config that stores name, width, height... etc
  */
  static MachineSettings& getInstance() {
    static MachineSettings sInstance;
    return sInstance;
  }
  struct MachineParam {
    enum class OriginType {
      /**
       *    RearLeft -------- RearRight
       *       |                 |
       *       |                 |
       *       |                 |
       *    FrontLeft ------ FrontRight
       */
      RearLeft,  // Machine origin at the "top left" in canvas coordinate
      RearRight, // Machine origin at the "top right" in canvas coordinate
      FrontLeft, // Machine origin at the "bottom left" in canvas coordinate
      FrontRight // Machine origin at the "bottom right" in canvas coordinate
    };
    enum class BoardType {
      GRBL_2020,
      BSL_2024,
      FLUX_2020,
      M2NANO_7,
      RUIDA_2020,
    };
  public:
    QString id;
    QString name;
    QString brand;
    QString model;
    QString icon_url;

    BoardType board_type;
    OriginType origin;
    int width;
    int height;
    double travel_speed;
    char rotary_axis;
    bool home_on_start;
    QPointF red_pointer_offset;
    bool is_high_speed_mode;//for lazervida

    MachineParam() :
      name("Default"),
      board_type(BoardType::GRBL_2020),
      origin(OriginType::RearLeft),
      width(100),
      height(100),
      travel_speed(100),
      rotary_axis('Y'),
      home_on_start(true),
      is_high_speed_mode(false) {}

    static MachineParam fromJson(const QJsonObject &obj);
    QJsonObject toJson() const;
    QIcon icon() const;
  };
  const QList<MachineParam> &getMachines();
  void setMachines(const QList<MachineParam> &new_machines);
  void addMachine(MachineParam mach);
  void clearMachines();
  void save();
  void loadJson(const QJsonObject &obj);
  QJsonObject toJson();
  static MachineParam findPreset(QString brand, QString model);
  static QList<MachineParam> database();
  MachineParam getTargetMachine(int machine_index);
  Q_INVOKABLE static QStringList brands();
  Q_INVOKABLE static QStringList models(QString brand);
  void setMachineRange(int machine_index, int width, int height);
  void setMachineTravelSpeed(int machine_index, double speed);
  void setMachineRotaryAxis(int machine_index, char axis);
  void setMachineHightSpeedMode(int machine_index, bool is_hight_mode);
  void setMachineStartWithHome(int machine_index, bool start_with_home);

Q_SIGNALS:
  void saveMachines(QJsonObject save_obj);

private:
  static QList<MachineParam> machineDatabase_; // not important, just a cache for lazy loading
  QList<MachineParam> machines_;
  QMutex machines_mutex_;
};
