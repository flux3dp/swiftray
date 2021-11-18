#include <QQuickItem>
#include <QQuickWidget>
#include <QTimer>
#include <QDebug>
#include "jogging-panel.h"
#include "ui_jogging-panel.h"

JoggingPanel::JoggingPanel(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::JoggingPanel)
{
    ui->setupUi(this);
    qmlRegisterType<MaintenanceController>("MaintenanceController", 1, 0, "MaintenanceController");
    ui->maintenanceController->setSource(QUrl("qrc:/src/windows/qml/MaintenanceDialog.qml"));
    ui->maintenanceController->show();
}

JoggingPanel::~JoggingPanel() {
    delete ui;
}

void JoggingPanel::close() {
  QDialog::close();
}
