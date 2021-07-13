#include "spooler-panel.h"
#include "ui_spooler-panel.h"

SpoolerPanel::SpoolerPanel(QWidget *parent) :
     QFrame(parent),
     ui(new Ui::SpoolerPanel),
     BaseContainer() {
  ui->setupUi(this);
  initializeContainer();
}

SpoolerPanel::~SpoolerPanel() {
  delete ui;
}
