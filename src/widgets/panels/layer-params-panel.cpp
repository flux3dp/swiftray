#include <QDebug>
#include "layer-params-panel.h"
#include "ui_layer-params-panel.h"
#include <windows/preset-manager.h>
#include <settings/preset-settings.h>
#include <document.h>
#include <windows/mainwindow.h>

LayerParamsPanel::LayerParamsPanel(QWidget *parent, MainWindow *main_window) :
     QFrame(parent),
     ui(new Ui::LayerParamsPanel),
     main_window_(main_window),
     layer_(nullptr),
     preset_previous_index_(0),
     BaseContainer() {
  ui->setupUi(this);
  initializeContainer();
  updateMovingComboBox();
}

LayerParamsPanel::~LayerParamsPanel() {
  delete ui;
}

void LayerParamsPanel::loadStyles() {
}

void LayerParamsPanel::loadSettings() {
  QString machine_model = main_window_->canvas()->document().settings().machine_model;
  qInfo() << "Loading model" << machine_model;
  if (preset_settings_->presets().size() > 0) {
    ui->presetComboBox->clear();
    for (auto &param: preset_settings_->currentPreset().params) {
      ui->presetComboBox->addItem(param.name, param.toJson());
    }
  }
  ui->presetComboBox->addItem(tr("Custom"));
  ui->presetComboBox->addItem(tr("More..."));
}

void LayerParamsPanel::registerEvents() {
  connect(preset_settings_, &PresetSettings::currentIndexChanged, [=]() {
    int index_to_recover = preset_previous_index_;
    loadSettings();
    if (index_to_recover > ui->presetComboBox->count() - 3) {
      setToCustom();
    } else {
      ui->presetComboBox->setCurrentIndex(index_to_recover);
    }
  });
  connect(ui->powerSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [=](int strength) {
    if (layer_ != nullptr) layer_->setStrength(strength);
    setToCustom();
  });
  connect(ui->speedSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [=](int speed) {
    if (layer_ != nullptr) layer_->setSpeed(speed);
    setToCustom();
  });
  connect(ui->repeatSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [=](int repeat) {
    if (layer_ != nullptr) layer_->setRepeat(repeat);
    setToCustom();
  });
  connect(ui->presetComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index) {
    if (index == ui->presetComboBox->count() - 1 && index > 0) {
      preset_manager_ = new PresetManager(this);
      int index_to_recover = preset_previous_index_;
      if (preset_manager_->exec() == 1) {
        preset_manager_->save();
        emit main_window_->presetSettingsChanged();
        loadSettings();
      }
      ui->presetComboBox->setCurrentIndex(index_to_recover);
      if (layer_ != nullptr) layer_->setParameterIndex(index_to_recover);
    } else if (index >= 0 && index < ui->presetComboBox->count() - 2) {
      auto p = PresetSettings::Param::fromJson(ui->presetComboBox->itemData(index).toJsonObject());
      ui->powerSpinBox->blockSignals(true);
      ui->speedSpinBox->blockSignals(true);
      ui->repeatSpinBox->blockSignals(true);
      ui->powerSpinBox->setValue(p.power);
      ui->speedSpinBox->setValue(p.speed);
      ui->repeatSpinBox->setValue(p.repeat);
      ui->powerSpinBox->blockSignals(false);
      ui->speedSpinBox->blockSignals(false);
      ui->repeatSpinBox->blockSignals(false);
      layer_->setStrength(p.power);
      layer_->setSpeed(p.speed);
      layer_->setRepeat(p.repeat);
      preset_previous_index_ = index;
      layer_->setParameterIndex(index);
    } else {
      preset_previous_index_ = index;
      layer_->setParameterIndex(index);
    }
  });
  connect(ui->movingComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index) {
    if(index > 0) {
      LayerPtr new_layer;
      auto cmd = Commands::Joined();
      for (auto &layer: main_window_->canvas()->document().layers()) {
        if(layer->name() == ui->movingComboBox->currentText()) {
          new_layer = layer;
          break;
        }
      }
      QList<ShapePtr> selected_items;
      for (auto &item: main_window_->canvas()->document().selections()) {
        if (item->layer() == new_layer.get()) {
          selected_items << item;
          continue;
        }
        ShapePtr clone_item = item->clone();
        cmd << Commands::AddShape(new_layer.get(), clone_item);
        cmd << Commands::RemoveShape(item);
        selected_items << clone_item;
      }
      cmd << Commands::Select(&(main_window_->canvas()->document()), selected_items);
      main_window_->canvas()->document().execute(cmd);
      main_window_->canvas()->setActiveLayer(new_layer);
    }
  });
}

void LayerParamsPanel::updateMovingComboBox() {
  ui->movingComboBox->blockSignals(true);
  ui->movingComboBox->clear();
  ui->movingComboBox->blockSignals(false);
  if (main_window_->canvas()->document().layers().length() > 1) {
    ui->movingComboBox->addItem(layer_->name());
    for (auto &layer: main_window_->canvas()->document().layers()) {
      if(layer.get() != layer_) {
        ui->movingComboBox->addItem(layer->name());
      }
    }

    if (ui->movingComboBox->count() > main_window_->canvas()->document().layers().size()) {
      ui->movingComboBox->removeItem(0);
    }
  }
}

void LayerParamsPanel::updateLayer(Layer *layer) {
  int previous_index = layer->parameterIndex();
  layer_ = layer;
  ui->presetParamLabel->setText(tr("Parameter Settings") + "("+ layer->name() + ")");
  ui->powerSpinBox->setValue(layer->power());
  ui->speedSpinBox->setValue(layer->speed());
  ui->repeatSpinBox->setValue(layer->repeat());
  ui->presetComboBox->setCurrentIndex(previous_index);
  updateMovingComboBox();
}

/*
  To avoid confusing with the recommended parameters,
  we change the preset combobox to `custom` whenever user changes the parameters.
*/
void LayerParamsPanel::setToCustom() {
  ui->presetComboBox->setCurrentIndex(ui->presetComboBox->count() - 2);
}
