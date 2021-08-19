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

class MachineSettings : public QObject {
Q_OBJECT
public:
  struct MachineSet {
    enum class OriginType {
      RearLeft,
      RearRight,
      FrontLeft,
      FrontRight
    };
    enum class BoardType {
      FLUX_2020,
      GRBL_2020,
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
    int width;
    int height;
    bool home_on_start;
    QPointF red_pointer_offset;

    static MachineSet fromJson(const QJsonObject &obj);

    QJsonObject toJson() const;

    QIcon icon() const;
  };

  MachineSettings();

  QList<MachineSet> &machines();

  void save();

  static MachineSet findPreset(QString brand, QString model);

  static QList<MachineSet> database();

  Q_INVOKABLE static QStringList brands();

  Q_INVOKABLE static QStringList models(QString brand);

private:
  static QList<MachineSet> machineDatabase_;

  void loadJson(const QJsonObject &obj);

  QJsonObject toJson();

  QList<MachineSet> machines_;
};
