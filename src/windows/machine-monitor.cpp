#include "machine-monitor.h"
#include "ui_machine-monitor.h"

MachineMonitor::MachineMonitor(QWidget *parent) :
     QDialog(parent),
     ui(new Ui::MachineMonitor),
     BaseContainer() {
  ui->setupUi(this);
  initializeContainer();
}

MachineMonitor::~MachineMonitor() {
  delete ui;
}
