#include "doc-panel.h"
#include "ui_doc-panel.h"
#include <settings/machine-settings.h>
#include <settings/preset-settings.h>

DocPanel::DocPanel(QWidget *parent, Canvas *canvas) :
     QFrame(parent),
     canvas_(canvas),
     ui(new Ui::DocPanel) {
  ui->setupUi(this);
  loadSettings();
  registerEvents();
}

void DocPanel::loadSettings() {
  MachineSettings settings;
  ui->modelComboBox->clear();
  for (auto &mach : settings.machines_) {
    ui->modelComboBox->addItem(QIcon(mach.icon), " " + mach.name, mach.toJson());
  }

  PresetSettings preset_settings;
  ui->presetComboBox->clear();
  for (auto &preset : preset_settings.presets()) {
    ui->presetComboBox->addItem(preset.name);
  }
}

void DocPanel::registerEvents() {
  ui->advanceFeatureToggle->setContent(ui->fluxFeatures);
}

DocPanel::~DocPanel() {
  delete ui;
}
