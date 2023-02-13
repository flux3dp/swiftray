#include <QDialog>
#include <QQuickItem>
#include <windows/mainwindow.h>
#include "welcome-dialog.h"

WelcomeDialog::WelcomeDialog(QWidget *parent) :
     QDialog(parent),
     widget_(new QQuickWidget(this)) {
  qmlRegisterType<MachineSettings>("MachineSettings", 1, 0, "MachineSettings");
  widget_->setSource(QUrl("qrc:/src/windows/qml/WelcomeDialog.qml"));
  widget_->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);
  QObject::connect(widget_->rootObject(), SIGNAL(setupComplete()),
                   this, SLOT(close()));
  QObject::connect(widget_->rootObject(), SIGNAL(createStandardProfile(QString, QString)),
                   this, SLOT(createStandardProfile(QString, QString)));
  QObject::connect(widget_->rootObject(), SIGNAL(createOtherProfile(QString, int, int, int)),
                   this, SLOT(createOtherProfile(QString, int, int, int)));
}

void WelcomeDialog::createStandardProfile(const QString brand, const QString model) {
  qInfo() << "Standard Profile" << brand << model;
  auto m = MachineSettings::findPreset(brand, model);
  assert(!m.brand.isEmpty());
  MachineSettings* machine_settings = &MachineSettings::getInstance();
  m.name = model.contains(brand) ? model : (brand + " " + model);
  std::size_t found = m.name.toStdString().find("Lazervida");
  if(found!=std::string::npos) {
    m.is_high_speed_mode = true;
  }
  machine_settings->addMachine(m);
  machine_settings->save();
  Q_EMIT updateCurrentMachineIndex(machine_settings->getMachines().size()-1);
}

void WelcomeDialog::createOtherProfile(const QString name, int width, int height, int origin) {
  qInfo() << "Other Profile" << name << width << height << origin;
  MachineSettings* machine_settings = &MachineSettings::getInstance();
  MachineSettings::MachineParam m;
  m.name = name;
  m.width = width;
  m.height = height;
  m.origin = (MachineSettings::MachineParam::OriginType) origin;
  std::size_t found = m.name.toStdString().find("Lazervida");
  if(found!=std::string::npos) {
    m.is_high_speed_mode = true;
  }
  machine_settings->addMachine(m);
  machine_settings->save();
  Q_EMIT updateCurrentMachineIndex(machine_settings->getMachines().size()-1);
}

void WelcomeDialog::close() {
  QDialog::close();
}
