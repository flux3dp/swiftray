#include <windows/osxwindow.h>
#include <QDebug>
#include <windows/gcode-player.h>
#include "maintenance-controller.h"

#ifndef Q_OS_IOS

#include <QTimer>
#include <QSerialPortInfo>
#include <connection/serial-port.h>
#include <QtMath>
#include <globals.h>

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

void MaintenanceController::laser() {
  QString job_str;
  if (is_laser_on_) {
    job_str = "G1S0\nM5";
    qInfo() << "laser off!";
    is_laser_on_ = false;
  } else {
    job_str = "$X\nM3\nG1F1000\nG1S10";
    is_laser_on_ = true;
    qInfo() << "laser on!";
  }
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
  if (!serial_port.isOpen()) {
    return;
  }
  QStringList cmd_list = job_str.split("\n");
  // Directly access serial port
  // TODO: Wait for ok for each cmd
  //       (Connect the responseReceive signal of SerialPort)
  for (auto cmd: cmd_list) {
    serial_port.write((cmd + "\n"));
  }
}

void MaintenanceController::testLog(const QString &str) const {
  qInfo() << "Test! MaintenanceController: " << str;
}

void MaintenanceController::loadSerialPorts() {
#ifndef Q_OS_IOS
  const auto infos = QSerialPortInfo::availablePorts();
  if (!infos.isEmpty()) {
    port_name_ = infos.last().portName();
  }
#endif
}
