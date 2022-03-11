#include <QImage>
#include <QIcon>
#include <settings/machine-settings.h>
#include <settings/preset-settings.h>
#include <windows/mainwindow.h>
#include "doc-panel.h"
#include "ui_doc-panel.h"

DocPanel::DocPanel(QWidget *parent, MainWindow *main_window) :
     QFrame(parent),
     main_window_(main_window),
     ui(new Ui::DocPanel),
     BaseContainer() {
  ui->setupUi(this);

  updateScene();

  // Fill data for DPI combobox items
  Q_ASSERT_X(ui->dpiComboBox->count() == 4, "doc-panel", "The item count of dpi combobox must match our expectation.");
  ui->dpiComboBox->setItemData(0, qreal{250}); // Low
  ui->dpiComboBox->setItemData(1, qreal{500}); // Mid
  ui->dpiComboBox->setItemData(2, qreal{750}); // High
  ui->dpiComboBox->setItemData(3, qreal{1000}); // Ultra High

  initializeContainer();
}

DocPanel::~DocPanel() {
  QSettings settings;
  settings.setValue("defaultMachine", ui->machineComboBox->currentText());
  delete ui;
}

/**
 * @brief Load document dpi settings into doc panel dpi combobox
 */
void DocPanel::syncDPISettingsUI() {
  bool convert_result;
  qreal dpi_value = main_window_->canvas()->document().settings().dpi; // Get dpi settings from document var
  for (auto idx = 0; idx < ui->dpiComboBox->count(); idx++) {
    qreal combo_box_item_value = ui->dpiComboBox->itemData(idx).toReal(&convert_result);
    Q_ASSERT_X(convert_result, "doc-panel", "itemData of DPI combobox must be a real number.");
    if (dpi_value <= combo_box_item_value) {
      ui->dpiComboBox->setCurrentIndex(idx);
      break;
    }
    if (idx == (ui->dpiComboBox->count() - 1)) {
      ui->dpiComboBox->setCurrentIndex(0); // no match -> set to default index 0
    }
  }
}

/**
 * @brief Load document advanced settings into doc panel advanced settings checkboxes
 */
void DocPanel::syncAdvancedSettingsUI() {
  ui->useAutoFocus->setCheckState(main_window_->canvas()->document().settings().use_af ? Qt::Checked : Qt::Unchecked);
  ui->useDiode->setCheckState(main_window_->canvas()->document().settings().use_diode ? Qt::Checked : Qt::Unchecked);
  ui->useRotary->setCheckState(main_window_->canvas()->document().settings().use_rotary ? Qt::Checked : Qt::Unchecked);
  ui->useOpenBottom->setCheckState(main_window_->canvas()->document().settings().use_open_bottom ? Qt::Checked : Qt::Unchecked);
}

/**
 * @brief Update UI (panel & scene) with current settings
 */
void DocPanel::loadSettings() {
  QSettings settings;
  MachineSettings machine_settings;
  QString current_machine = settings.value("defaultMachine").toString();
  ui->machineComboBox->clear();
  for (auto &mach : machine_settings.machines()) {
    if (mach.name.isEmpty()) continue;
    ui->machineComboBox->addItem(mach.icon(), " " + mach.name, mach.toJson());
  }
  ui->machineComboBox->setCurrentText(current_machine);
  updateScene();

  PresetSettings* preset_settings = &PresetSettings::getInstance();
  ui->presetComboBox->clear();
  for (auto &preset : preset_settings->presets()) {
    ui->presetComboBox->addItem(preset.name);
  }

  // Load DPI setting (select appropriate dpi item index)
  syncDPISettingsUI();
  syncAdvancedSettingsUI();
}

void DocPanel::registerEvents() {
  ui->advanceFeatureToggle->setContent(ui->fluxFeatures);
  ui->advanceFeatureToggle->hide();

  // Events of internal document settings changed trigger update in UI (panel & canvas)
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
  connect(main_window_->canvas(), &Canvas::docSettingsChanged, [=]() {
    loadSettings();
  });

  // Events of panel UI edit complete trigger save actions into internal document settings
  connect(ui->dpiComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index) {
    // TODO: Consider using model/view
    // sync document settings with dpi combo box data
    main_window_->canvas()->document().settings().dpi = ui->dpiComboBox->itemData(ui->dpiComboBox->currentIndex()).toReal();
  });

  // Advanced settings
  connect(ui->useAutoFocus, QOverload<int>::of(&QCheckBox::stateChanged), [=](int state) {
    main_window_->canvas()->document().settings().use_af = state == Qt::Checked ? true : false;
  });
  connect(ui->useDiode, QOverload<int>::of(&QCheckBox::stateChanged), [=](int state) {
    main_window_->canvas()->document().settings().use_diode = state == Qt::Checked ? true : false;
  });
  connect(ui->useRotary, QOverload<int>::of(&QCheckBox::stateChanged), [=](int state) {
    main_window_->canvas()->document().settings().use_rotary = state == Qt::Checked ? true : false;
  });
  connect(ui->useOpenBottom, QOverload<int>::of(&QCheckBox::stateChanged), [=](int state) {
    main_window_->canvas()->document().settings().use_open_bottom = state == Qt::Checked ? true : false;
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
