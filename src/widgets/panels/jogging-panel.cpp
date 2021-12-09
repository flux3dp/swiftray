#include "jogging-panel.h"
#include "ui_jogging-panel.h"
#include <QQuickItem>
#include <QQuickWidget>
#include <windows/mainwindow.h>

JoggingPanel::JoggingPanel(QWidget *parent, MainWindow *main_window) :
     QFrame(parent),
     main_window_(main_window),
     ui(new Ui::JoggingPanel),
     BaseContainer() {
  assert(parent != nullptr && main_window != nullptr);
  ui->setupUi(this);
  initializeContainer();
  qmlRegisterType<MaintenanceController>("MaintenanceController", 1, 0, "MaintenanceController");
  ui->maintenanceController->setSource(QUrl("qrc:/src/windows/qml/MaintenanceDialog.qml"));
  ui->maintenanceController->show();
}

JoggingPanel::~JoggingPanel() {
  delete ui;
}
