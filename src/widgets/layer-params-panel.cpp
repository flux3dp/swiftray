#include <QDebug>
#include "layer-params-panel.h"
#include "ui_layer-params-panel.h"
#include <widgets/spinbox-helper.h>
#include "preset-manager.h"
#include <settings/param-settings.h>

LayerParamsPanel::LayerParamsPanel(QWidget *parent) :
     QFrame(parent),
     ui(new Ui::LayerParamsPanel),
     param_settings_(make_unique<ParamSettings>()) {
  ui->setupUi(this);
  loadStyles();
  registerEvents();
  loadSettings();
}

LayerParamsPanel::~LayerParamsPanel() {
  delete ui;
}

void LayerParamsPanel::loadStyles() {
  ((SpinBoxHelper<QSpinBox> *) ui->spinBoxPower)->lineEdit()->setStyleSheet("padding: 0 8px;");
  ((SpinBoxHelper<QSpinBox> *) ui->spinBoxRepeat)->lineEdit()->setStyleSheet("padding: 0 8px;");
  ((SpinBoxHelper<QSpinBox> *) ui->spinBoxSpeed)->lineEdit()->setStyleSheet("padding: 0 8px;");
}

void LayerParamsPanel::loadSettings() {
  ui->presetComboBox->clear();
  for (auto &param: param_settings_->params) {
    ui->presetComboBox->addItem(param.name, param.toJson());
  }
  ui->presetComboBox->addItem("More...");
}

void LayerParamsPanel::registerEvents() {
  connect(ui->spinBoxPower, QOverload<int>::of(&QSpinBox::valueChanged), [=](int strength) {
    if (layer_ != nullptr) layer_->setStrength(strength);
  });
  connect(ui->spinBoxSpeed, QOverload<int>::of(&QSpinBox::valueChanged), [=](int speed) {
    if (layer_ != nullptr) layer_->setSpeed(speed);
  });
  connect(ui->spinBoxRepeat, QOverload<int>::of(&QSpinBox::valueChanged), [=](int repeat) {
    if (layer_ != nullptr) layer_->setRepeat(repeat);
  });
  connect(ui->presetComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index) {
    if (index == ui->presetComboBox->count() - 1) {
      preset_manager_ = make_unique<PresetManager>(this);
      preset_manager_->show();
    } else {
      auto p = ParamSettings::ParamSet::fromJson(ui->presetComboBox->itemData(index).toJsonObject());
      ui->spinBoxPower->setValue(p.power);
      ui->spinBoxSpeed->setValue(p.speed);
      ui->spinBoxRepeat->setValue(p.repeat);
    }
  });
}

void LayerParamsPanel::updateLayer(Layer *layer) {
  layer_ = layer;
  ui->spinBoxPower->setValue(layer->power());
  ui->spinBoxSpeed->setValue(layer->speed());
  ui->spinBoxRepeat->setValue(layer->repeat());
}
