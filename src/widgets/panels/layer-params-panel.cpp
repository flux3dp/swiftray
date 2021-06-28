#include <QDebug>
#include "layer-params-panel.h"
#include "ui_layer-params-panel.h"
#include <windows/preset-manager.h>
#include <widgets/components/spinbox-helper.h>
#include <settings/preset-settings.h>
#include <canvas/canvas.h>
#include <document.h>

LayerParamsPanel::LayerParamsPanel(QWidget *parent, Canvas *canvas) :
     QFrame(parent),
     ui(new Ui::LayerParamsPanel),
     canvas_(canvas),
     layer_(nullptr),
     preset_previous_index_(0) {
  ui->setupUi(this);
  loadStyles();
  registerEvents();
  loadSettings();
}

LayerParamsPanel::~LayerParamsPanel() {
  delete ui;
}

void LayerParamsPanel::loadStyles() {
}

void LayerParamsPanel::loadSettings() {
  QString machine_model = canvas_->document().settings().machine_model;
  qInfo() << "Loading model" << machine_model;
  PresetSettings preset;
  if (preset.presets().size() > 0) {
    ui->presetComboBox->clear();
    for (auto &param: preset.presets().first().params) {
      ui->presetComboBox->addItem(param.name, param.toJson());
    }
  }

  ui->presetComboBox->addItem("More...");
}

void LayerParamsPanel::registerEvents() {
  connect(ui->powerSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [=](int strength) {
    if (layer_ != nullptr) layer_->setStrength(strength);
  });
  connect(ui->speedSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [=](int speed) {
    if (layer_ != nullptr) layer_->setSpeed(speed);
  });
  connect(ui->repeatSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [=](int repeat) {
    if (layer_ != nullptr) layer_->setRepeat(repeat);
  });
  connect(ui->presetComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index) {
    if (index == ui->presetComboBox->count() - 1 && index > 0) {
      preset_manager_ = new PresetManager(this);
      if (preset_manager_->exec() == 1) {
        preset_manager_->save();
      }
      // TODO (Check if index available, or use string as recover index);
      loadSettings();
      ui->presetComboBox->setCurrentIndex(preset_previous_index_);
    } else {
      auto p = PresetSettings::Param::fromJson(ui->presetComboBox->itemData(index).toJsonObject());
      ui->powerSpinBox->setValue(p.power);
      ui->speedSpinBox->setValue(p.speed);
      ui->repeatSpinBox->setValue(p.repeat);
    }
    preset_previous_index_ = index;
  });
}

void LayerParamsPanel::updateLayer(Layer *layer) {
  layer_ = layer;
  ui->powerSpinBox->setValue(layer->power());
  ui->speedSpinBox->setValue(layer->speed());
  ui->repeatSpinBox->setValue(layer->repeat());
}
