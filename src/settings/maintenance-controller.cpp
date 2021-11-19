#include <windows/osxwindow.h>
#include <QDebug>
#include <windows/gcode-player.h>
#include "maintenance-controller.h"

#ifndef Q_OS_IOS

#include <QTimer>
#include <QSerialPortInfo>
#include <SerialPort/SerialPort.h>
#include <QtMath>

#endif

MaintenanceController::MaintenanceController() {
  QTimer *timer = new QTimer(this);
  connect(timer, &QTimer::timeout, this, &MaintenanceController::loadSerialPorts);
  timer->start(3000);
}

MaintenanceController::~MaintenanceController() {
  if (!jobs_.isEmpty()) {
    for (int i = 0; i < jobs_.length(); i += 1) {
      delete jobs_[i];
    }
  }
}

void MaintenanceController::homing() {
  QString job_str = "\n$H";
  qInfo() << "Homing!";
  sendJob(job_str);
}

void MaintenanceController::laserPulse() {
  QString job_str = "M5\nG91\nM3S300\nG1F1200S300\nG1X0Y0\nG1F1200S0\nG1X0Y0\nG90";
  qInfo() << "laser pulse!";
  sendJob(job_str);
}

void MaintenanceController::moveRelatively(float x, float y) {
  QString job_str = "M5\nG91\nG1F1200S0\nG1X" + QString::number(x) + "Y" + QString::number(y) + "\nG90";
  qInfo() << "moveRelatively, " << "X: " << x << " Y: " << y;
  sendJob(job_str);
}

void MaintenanceController::moveTo(float x, float y) {
  QString job_str = "M5\nG90\nG1F1200S0\nG1X" + QString::number(x) + "Y" + QString::number(y) + "\nG90";
  qInfo() << "moveTo, " << "X: " << x << " Y: " << y;
  sendJob(job_str);
}

void MaintenanceController::moveX(float x) {
  QString job_str = "M5\nG90\nG1F1200S0\nG1X" + QString::number(x) + "\nG90";
  qInfo() << "moveX: " << x ;
  sendJob(job_str);
}

void MaintenanceController::moveY(float y) {
  QString job_str = "M5\nG90\nG1F1200S0\nG1Y" + QString::number(y) + "\nG90";
  qInfo() << "moveY: " << y;
  sendJob(job_str);
}

void MaintenanceController::sendJob(QString &job_str) {
  if (!SerialPort::getInstance().isConnected()) {
    return;
  }
  QStringList cmd_list = job_str.split("\n");
  // Directly access serial port
  // TODO: Wait for ok for each cmd
  //       (Connect the responseReceive signal of SerialPort)
  for (auto cmd: cmd_list) {
    SerialPort::getInstance().write_some((cmd + "\n").toStdString());
  }
}

void MaintenanceController::testLog(const QString &str) const {
  qInfo() << "Test! MaintenanceController: " << str;
}

void MaintenanceController::loadSerialPorts() {
#ifndef Q_OS_IOS
  const auto infos = QSerialPortInfo::availablePorts();
  port_name_ = infos.last().portName();
#endif
}
