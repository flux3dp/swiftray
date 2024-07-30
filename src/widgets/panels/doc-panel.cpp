#include <QImage>
#include <QIcon>
#include <settings/machine-settings.h>
#include <settings/preset-settings.h>
#include <windows/mainwindow.h>
#include "doc-panel.h"
#include "ui_doc-panel.h"
#include <globals.h>

DocPanel::DocPanel(QWidget *parent, MainWindow *main_window) :
     QFrame(parent),
     main_window_(main_window),
     ui(new Ui::DocPanel),
     BaseContainer() {
  ui->setupUi(this);

  // Fill data for DPI combobox items
  Q_ASSERT_X(ui->dpiComboBox->count() == 4, "doc-panel", "The item count of dpi combobox must match our expectation.");
  ui->dpiComboBox->setItemData(0, qreal{125*qPow(2,0)}); // Low (125)
  ui->dpiComboBox->setItemData(1, qreal{125*qPow(2,1)}); // Mid (250)
  ui->dpiComboBox->setItemData(2, qreal{125*qPow(2,2)}); // High (500)
  ui->dpiComboBox->setItemData(3, qreal{125*qPow(2,3)}); // Ultra High (1000)

  initializeContainer();
}

DocPanel::~DocPanel() {
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
  // Load DPI setting (select appropriate dpi item index)
  syncDPISettingsUI();
  syncAdvancedSettingsUI();
  if(ui->rotaryCheckBox->checkState()) {
    ui->speedSpinBox->setEnabled(false);
    ui->rotarySpinBox->setEnabled(true);
  } else {
    ui->speedSpinBox->setEnabled(true);
    ui->rotarySpinBox->setEnabled(false);
  }
}

void DocPanel::registerEvents() {
  ui->advanceFeatureToggle->setContent(ui->fluxFeatures);
  ui->advanceFeatureToggle->hide();

  // Events of internal document settings changed trigger update in UI (panel & canvas)
  connect(ui->presetComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index) {
    Q_EMIT updatePresetIndex(index);
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
  connect(ui->rotaryCheckBox, &QCheckBox::stateChanged, [=](int state) {
    if(state) {
      ui->speedSpinBox->setEnabled(false);
      ui->rotarySpinBox->setEnabled(true);
    } else {
      ui->speedSpinBox->setEnabled(true);
      ui->rotarySpinBox->setEnabled(false);
    }
    Q_EMIT rotaryModeChange(state);
  });
  connect(ui->speedSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=](double value) {
    Q_EMIT updateTravelSpeed(value);
  });
  connect(ui->rotarySpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=](double value) {
    Q_EMIT updateRotarySpeed(value);
  });
}

void DocPanel::setRotaryMode(bool is_rotary_mode) {
  ui->rotaryCheckBox->blockSignals(true);
  if(is_rotary_mode) {
    ui->rotaryCheckBox->setCheckState(Qt::Checked);
    ui->speedSpinBox->setEnabled(false);
    ui->rotarySpinBox->setEnabled(true);
  }
  else {
    ui->rotaryCheckBox->setCheckState(Qt::Unchecked);
    ui->speedSpinBox->setEnabled(true);
    ui->rotarySpinBox->setEnabled(false);
  }
  ui->rotaryCheckBox->blockSignals(false);
}

void DocPanel::setTravelSpeed(double travel_speed) {
  ui->speedSpinBox->blockSignals(true);
  ui->speedSpinBox->setValue(travel_speed);
  ui->speedSpinBox->blockSignals(false);
}

void DocPanel::setRotarySpeed(double rotary_speed) {
  ui->rotarySpinBox->blockSignals(true);
  ui->rotarySpinBox->setValue(rotary_speed);
  ui->rotarySpinBox->blockSignals(false);
}

void DocPanel::setPresetSelectLock(bool enable) {
  ui->presetComboBox->setEnabled(enable);
}

void DocPanel::hideEvent(QHideEvent *event) {
  Q_EMIT panelShow(false);
}

void DocPanel::showEvent(QShowEvent *event) {
  Q_EMIT panelShow(true);
}

void DocPanel::setPresetIndex(int preset_index) {
  PresetSettings* settings = &PresetSettings::getInstance();
  ui->presetComboBox->blockSignals(true);
  ui->presetComboBox->clear();
  for (auto &preset : settings->presets()) {
    ui->presetComboBox->addItem(preset.name);
  }
  ui->presetComboBox->setCurrentIndex(preset_index);
  ui->presetComboBox->blockSignals(false);
}
