#pragma once

#include <QString>
#include <QFile>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>
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
  /**
      \class MachineSet
      \brief The MachineSet class represents a machine config that stores name, width, height... etc
  */
  struct MachineSet {
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
      FLUX_2020,
      M2NANO_7,
      RUIDA_2020
    };
  public:
    QString id;
    QString name;
    QString brand;
    QString model;
    QString icon_url;

    BoardType board_type;
    OriginType origin;
    int width = 0;
    int height = 0;
    bool home_on_start;
    QPointF red_pointer_offset;

    static MachineSet fromJson(const QJsonObject &obj);

    QJsonObject toJson() const;

    QIcon icon() const;
  };

  MachineSettings();

  const QList<MachineSet> &machines();

  void addMachine(MachineSet mach);

  void clearMachines();

  void save();

  static MachineSet findPreset(QString brand, QString model);

  static QList<MachineSet> database();

  Q_INVOKABLE static QStringList brands();

  Q_INVOKABLE static QStringList models(QString brand);

private:
  static QList<MachineSet> machineDatabase_; // not important, just a cache for lazy loading

  void loadJson(const QJsonObject &obj);

  QJsonObject toJson();

  QList<MachineSet> machines_;
  QMutex machines_mutex_;
};
