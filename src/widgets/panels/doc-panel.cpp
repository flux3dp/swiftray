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
  connect(ui->modelComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index) {
    auto data = ui->modelComboBox->itemData(index);
    auto machine = MachineSettings::MachineSet::fromJson(data.toJsonObject());
    // TODO (change width/height to QSize)
    main_window_->canvas()->document().setWidth(machine.width * 10);
    main_window_->canvas()->document().setHeight(machine.height * 10);
    main_window_->canvas()->resize();
  });

  connect(main_window_, &MainWindow::presetSettingsChanged, [=]() {
    loadSettings();
  });
}

DocPanel::~DocPanel() {
  delete ui;
}
