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
  MachineSettings::MachineSet newMach = MachineSettings::findPreset(brand, model);
  assert(!newMach.brand.isEmpty());
  MachineSettings m;
  newMach.name = brand + " " + model;
  m.machines() << newMach;
  m.save();
  emit settingsChanged();
}

void WelcomeDialog::createOtherProfile(const QString name, int width, int height, int origin) {
  qInfo() << "Other Profile" << name << width << height << origin;
  emit settingsChanged();
}

void WelcomeDialog::close() {
  QDialog::close();
}