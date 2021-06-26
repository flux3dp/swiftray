#include "doc-panel.h"
#include "ui_doc-panel.h"

DocPanel::DocPanel(QWidget *parent, Canvas *canvas) :
     QFrame(parent),
     canvas_(canvas),
     ui(new Ui::DocPanel) {
  ui->setupUi(this);
  registerEvents();
}

void DocPanel::registerEvents() {
  ui->advanceFeatureToggle->setContent(ui->fluxFeatures);
}

DocPanel::~DocPanel() {
  delete ui;
}
