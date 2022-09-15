#include "jogging-panel.h"
#include "ui_jogging-panel.h"
#include <QQuickItem>
#include <QQuickWidget>
#include <settings/machine-settings.h>
#include <windows/mainwindow.h>
#include <windows/osxwindow.h>
#include <globals.h>

JoggingPanel::JoggingPanel(QWidget *parent, MainWindow *main_window) :
     QFrame(parent),
     main_window_(main_window),
     ui(new Ui::JoggingPanel),
     BaseContainer() {
  assert(parent != nullptr && main_window != nullptr);
  ui->setupUi(this);
  initializeContainer();
  ui->maintenanceController->setSource(QUrl("qrc:/src/windows/qml/MaintenanceDialog.qml"));
  ui->maintenanceController->rootContext()->setContextProperty("is_dark_mode", isDarkMode());
  ui->maintenanceController->show();

  ui->clearOriginBtn->hide();
  ui->setOriginBtn->hide();

  QObject::connect(ui->maintenanceController->rootObject(), SIGNAL(moveRelatively(int, int)),
                  this, SLOT(moveRelatively(int, int)));
  QObject::connect(ui->maintenanceController->rootObject(), SIGNAL(moveToCorner(int)),
                  this, SLOT(moveToCorner(int)));
  QObject::connect(ui->maintenanceController->rootObject(), SIGNAL(moveToEdge(int)),
                  this, SLOT(moveToEdge(int)));
  QObject::connect(ui->maintenanceController->rootObject(), SIGNAL(home()),
                  this, SLOT(home()));
  QObject::connect(ui->laserBtn, &QAbstractButton::clicked, this, &JoggingPanel::laser);
  QObject::connect(ui->laserPulseBtn, &QAbstractButton::clicked, this, &JoggingPanel::laserPulse);
  connect(ui->syncBtn, &QAbstractButton::clicked, [=]() {
    ui->moveXSpinBox->setValue(ui->currentXSpinBox->value());
    ui->moveYSpinBox->setValue(ui->currentYSpinBox->value());
  });
} 

void JoggingPanel::home() {
  if(!control_enable_) {
    return;
  }
  QString job_str = "\n$H";
  qInfo() << "Homing!";
  sendJob(job_str);
}

void JoggingPanel::laser() {
  if(!control_enable_) {
    return;
  }
  QString job_str;
  if (is_laser_on_) {
    job_str = "G1S0\nM5";
    qInfo() << "laser off!";
    is_laser_on_ = false;
  } else {
    job_str = "$X\nM3\nG1F1000\nG1S20";
    is_laser_on_ = true;
    qInfo() << "laser on!";
  }
  sendJob(job_str);
}

void JoggingPanel::laserPulse() {
  if(!control_enable_) {
    return;
  }
  QString job_str = "$X\nM5\nG91\nM3S300\nG1F1200S300\nG1X0Y0\nG1F1200S0\nG1X0Y0\nG90";
  qInfo() << "laser pulse!";
  sendJob(job_str);
}

void JoggingPanel::moveRelatively(int dir, int level) {
  if(!control_enable_) {
    return;
  }
  float magnitude = 0;
  QPointF movement(0,0);

  switch (level) {
    case 0:
      magnitude = 0.1;
      break;
    case 1:
      magnitude = 2;
      break;
    case 2:
      magnitude = 10;
      break;
  }

  switch (dir) {
    case 0:
      movement.setX(magnitude);
      break;
    case 1:
      movement.setY(-magnitude);
      break;
    case 2:
      movement.setX(-magnitude);
      break;
    case 3:
      movement.setY(magnitude);
      break;
  }

  switch (main_window_->currentMachine().origin) {
    case MachineSettings::MachineSet::OriginType::RearRight:
      // Canvas x axis direction is opposite to machine coordinate
      movement.setX(-movement.x());
      break;
    case MachineSettings::MachineSet::OriginType::FrontRight:
      // Canvas x, y axis directions are opposite to machine coordinate
      movement.setX(-movement.x());
      movement.setY(-movement.y());
      break;
    case MachineSettings::MachineSet::OriginType::FrontLeft:
      // Canvas y axis direction is opposite to machine coordinate
      movement.setY(-movement.y());
      break;
    default:  
      break;
  }

  QString job_str = "$X\nM5\n$J=G91 F1200 X" + QString::number(movement.x()) + "Y" + QString::number(movement.y());
  sendJob(job_str);
}

void JoggingPanel::moveToEdge(int dir) {
  if(!control_enable_) {
    return;
  }
  QPointF movement(0,0);
  QString job_str;

  switch (dir) {
    case 0:
      movement.setX(main_window_->currentMachine().width);
      movement = transformDirection(movement);
      job_str = "$X\nM5\n$J=G90 F1200 X" + QString::number(movement.x());
      break;
    case 1:
      movement.setY(0);
      movement = transformDirection(movement);
      job_str = "$X\nM5\n$J=G90 F1200 Y" + QString::number(movement.y());
      break;
    case 2:
      movement.setX(0);
      movement = transformDirection(movement);
      job_str = "$X\nM5\n$J=G90 F1200 X" + QString::number(movement.x());
      break;
    case 3:
      movement.setY(main_window_->currentMachine().height);
      movement = transformDirection(movement);
      job_str = "$X\nM5\n$J=G90 F1200 Y" + QString::number(movement.y());
      break;
  }

  sendJob(job_str);
}

void JoggingPanel::moveToCorner(int corner) {
  if(!control_enable_) {
    return;
  }
  QPointF movement(0,0);
  // corner A:0 B:1 C:2 D:4
  switch (corner) {
    case 0:
      movement.setX(0);
      movement.setY(0);
      break;
    case 1:
      movement.setX(main_window_->currentMachine().width);
      movement.setY(0);
      break;
    case 2:
      movement.setX(0);
      movement.setY(main_window_->currentMachine().height);
      break;
    case 3:
      movement.setX(main_window_->currentMachine().width);
      movement.setY(main_window_->currentMachine().height);
      break;
  }
  movement = transformDirection(movement);

  QString job_str = "$X\nM5\n$J=G90 F1200 X" + QString::number(movement.x()) + "Y" + QString::number(movement.y());
  sendJob(job_str);
}

QPointF JoggingPanel::transformDirection(QPointF movement) {
  float x, y;
  switch (main_window_->currentMachine().origin) {
    case MachineSettings::MachineSet::OriginType::RearRight:
      // Canvas x axis direction is opposite to machine coordinate
      x = main_window_->currentMachine().width - movement.x();
      y = movement.y();
      break;
    case MachineSettings::MachineSet::OriginType::FrontRight:
      // Canvas x, y axis directions are opposite to machine coordinate
      x = main_window_->currentMachine().width - movement.x();
      y = main_window_->currentMachine().height - movement.y();
      break;
    case MachineSettings::MachineSet::OriginType::RearLeft:
      // NORMAL canvas x, y axis directions are the same as machine coordinate
      x = movement.x();
      y = movement.y();
      break;
    case MachineSettings::MachineSet::OriginType::FrontLeft:
      // Canvas y axis direction is opposite to machine coordinate
      x = movement.x();
      y = main_window_->currentMachine().height - movement.y();
      break;
  }

  return QPointF(x, y);
}

void JoggingPanel::setControlEnable(bool control_enable) {
  control_enable_ = control_enable;
}

void JoggingPanel::hideEvent(QHideEvent *event) {
  emit panelShow(false);
}

void JoggingPanel::showEvent(QShowEvent *event) {
  emit panelShow(true);
}

void JoggingPanel::sendJob(QString &job_str) {
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

void JoggingPanel::setAxisPosition(double x_position, double y_position) {
  ui->currentXSpinBox->setValue(x_position);
  ui->currentYSpinBox->setValue(y_position);
}

JoggingPanel::~JoggingPanel() {
  delete ui;
}
