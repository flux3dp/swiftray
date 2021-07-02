#include "doc-panel.h"
#include "ui_doc-panel.h"
#include <settings/machine-settings.h>
#include <settings/preset-settings.h>
#include <windows/mainwindow.h>


DocPanel::DocPanel(QWidget *parent, MainWindow *main_window) :
     QFrame(parent),
     main_window_(main_window),
     ui(new Ui::DocPanel) {
  ui->setupUi(this);
  loadSettings();
  registerEvents();
  updateScene();
}

DocPanel::~DocPanel() {
  QSettings settings;
  settings.setValue("defaultMachine", ui->machineComboBox->currentText());
  delete ui;
}

void DocPanel::loadSettings() {
  QSettings settings;
  MachineSettings machine_settings;
  QString current_machine = settings.value("defaultMachine").toString();
  ui->machineComboBox->clear();
  for (auto &mach : machine_settings.machines_) {
    ui->machineComboBox->addItem(QIcon(mach.icon), " " + mach.name, mach.toJson());
  }
  ui->machineComboBox->setCurrentText(current_machine);
  updateScene();

  PresetSettings preset_settings;
  ui->presetComboBox->clear();
  for (auto &preset : preset_settings.presets()) {
    ui->presetComboBox->addItem(preset.name);
  }
}

void DocPanel::registerEvents() {
  ui->advanceFeatureToggle->setContent(ui->fluxFeatures);
  connect(ui->machineComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index) {
    if (index == -1) return;
    updateScene();
    QSettings settings;
    settings.setValue("defaultMachine", ui->machineComboBox->currentText());
  });

  connect(main_window_, &MainWindow::presetSettingsChanged, [=]() {
    loadSettings();
  });

  connect(main_window_, &MainWindow::machineSettingsChanged, [=]() {
    loadSettings();
  });
}

void DocPanel::updateScene() {
  if (ui->machineComboBox->count() == 0) return;
  auto data = ui->machineComboBox->itemData(ui->machineComboBox->currentIndex());
  auto machine = MachineSettings::MachineSet::fromJson(data.toJsonObject());
  // TODO (change width/height to QSize)
  main_window_->canvas()->document().setWidth(machine.width * 10);
  main_window_->canvas()->document().setHeight(machine.height * 10);
  main_window_->canvas()->resize();
}


MachineSettings::MachineSet DocPanel::currentMachine() {
  if (ui->machineComboBox->count() == 0) {
    MachineSettings::MachineSet m;
    m.origin = MachineSettings::MachineSet::OriginType::RearLeft;
    return m;
  }
  auto data = ui->machineComboBox->itemData(ui->machineComboBox->currentIndex());
  auto machine = MachineSettings::MachineSet::fromJson(data.toJsonObject());
  return machine;
}