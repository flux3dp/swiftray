#pragma once

#include <QString>
#include <QFile>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>
#include <QJsonDocument>
#include <QPointF>
#include <QIcon>
#include <QSerialPortInfo>
#include <QStringList>

#ifndef Q_OS_IOS

#include <QSerialPort>
#include <connection/serial-job.h>

#endif


/**
 *  \class MaintenanceController
 *  \brief Manage maintenance window controll
 *
 *  MaintenanceController utilize serial port to control the machine
*/
class MaintenanceController : public QObject {
Q_OBJECT
public:
  explicit MaintenanceController();

  ~MaintenanceController();

  Q_INVOKABLE void testLog(const QString &str) const;

  Q_INVOKABLE void connectSerialPort();

//  Q_INVOKABLE void send(const QString &str) const;

  Q_INVOKABLE void laserPulse();

  Q_INVOKABLE void moveRelatively(float x, float y);

  Q_INVOKABLE void moveTo(float x, float y);

  Q_INVOKABLE void moveX(float x);

  Q_INVOKABLE void moveY(float y);

  void sendJob(QString &job_str);

  void loadSerialPorts();

private:

  QString port_name_;

#ifndef Q_OS_IOS
  QList<SerialJob *> jobs_;
#endif
};
