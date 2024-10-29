#include "layer-params-panel.h"
#include "ui_layer-params-panel.h"
#include <document.h>
#include <settings/preset-settings.h>
#include <windows/mainwindow.h>
#include <windows/osxwindow.h>

#include <QDebug>

LayerParamsPanel::LayerParamsPanel(QWidget *parent, MainWindow *main_window) :
     QFrame(parent),
     ui(new Ui::LayerParamsPanel),
     main_window_(main_window),
     layer_(nullptr),
     BaseContainer() {
  ui->setupUi(this);
  initializeContainer();
  updateMovingComboBox();
  this->addInput({ui->powerSpinBox,
                 ui->speedSpinBox,
                 ui->repeatSpinBox,
                 ui->presetComboBox,
                 ui->backlashSpinBox,
                 ui->freqSpinBox,
                 ui->pulseWidthSpinBox,
                 ui->fillIntervalSpinBox,
                 ui->fillAngleSpinBox});
}

LayerParamsPanel::~LayerParamsPanel() {
  delete ui;
}

void LayerParamsPanel::loadStyles() {
  // Add floating buttons
  QPointF button_pos = ui->parameterFrame->mapToGlobal(ui->parameterFrame->geometry().topRight()) - QPointF(24, 0);
  add_layer_btn_ = new QToolButton(ui->parameterFrame);
  add_layer_btn_->setObjectName("btnAddLayer");
  add_layer_btn_->setCursor(Qt::PointingHandCursor);
  add_layer_btn_->setIcon(QIcon(isDarkMode() ? ":/resources/images/dark/icon-plus.png" : ":/resources/images/icon-plus.png"));
  add_layer_btn_->setIconSize(QSize(24, 24));
  add_layer_btn_->setGeometry(QRect(button_pos.x(), button_pos.y(), 24, 24));
  add_layer_btn_->raise();
  add_layer_btn_->show();
  ui->parameterFrame->setStyleSheet("\
    QToolButton#btnAddLayer:hover {\
      border: 0px;\
    }\
    QToolButton#btnAddLayer{ \
      border: 0px;\
    } \
  ");
}

void LayerParamsPanel::loadSettings() {
  // QString machine_model = main_window_->canvas()->document().settings().machine_model;
  // qInfo() << "Loading model" << machine_model;
}

void LayerParamsPanel::registerEvents() {
  connect(ui->powerSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=](double strength) {
    current_params_.strength = strength;
    Q_EMIT editLayerParams(current_params_);
    setToCustom();
  });
  connect(ui->speedSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=](double speed) {
    current_params_.speed = speed;
    Q_EMIT editLayerParams(current_params_);
    setToCustom();
  });
  connect(ui->repeatSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [=](int repeat) {
    current_params_.repeat = repeat;
    Q_EMIT editLayerParams(current_params_);
    setToCustom();
  });
  connect(ui->backlashSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=](double backlash) {
    current_params_.backlash = backlash;
    Q_EMIT editLayerParams(current_params_);
  });
  connect(ui->freqSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [=](int frequency) {
    current_params_.frequency = frequency;
    Q_EMIT editLayerParams(current_params_);
  });
  connect(ui->pulseWidthSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [=](int pulse_width) {
    current_params_.pulse_width = pulse_width;
    Q_EMIT editLayerParams(current_params_);
  });
  connect(ui->fillIntervalSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=](double fill_interval) {
    current_params_.fill_interval = fill_interval;
    Q_EMIT editLayerParams(current_params_);
  });
  connect(ui->fillAngleSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=](double fill_angle) {
    current_params_.fill_angle = fill_angle;
    Q_EMIT editLayerParams(current_params_);
  });
  connect(ui->presetComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index) {
    if (index == ui->presetComboBox->count() - 1) {
      Q_EMIT wakeupPresetManager();
    } else {
      Q_EMIT editParamIndex(index);
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
  connect(add_layer_btn_, &QAbstractButton::clicked, main_window_->canvas(), &Canvas::addEmptyLayer);
}

void LayerParamsPanel::resizeEvent(QResizeEvent *e) {
  if (add_layer_btn_ == nullptr) return;
  QPointF button_pos = ui->parameterFrame->geometry().topRight() - QPointF(24, 0);
  add_layer_btn_->setGeometry(QRect(button_pos.x(), button_pos.y(), 24, 24));
}

void LayerParamsPanel::updateMovingComboBox() {
  ui->movingComboBox->blockSignals(true);
  ui->movingComboBox->clear();
  ui->movingComboBox->blockSignals(false);
  if (main_window_->canvas()->document().layers().length() > 1) {
    ui->movingComboBox->setEnabled(true);
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
  else{
    ui->movingComboBox->setEnabled(false);
  }
}

void LayerParamsPanel::updateLayer(Layer *layer) {
  int previous_index = layer->parameterIndex();
  if(previous_index < 0) previous_index = ui->presetComboBox->count() - 2;
  layer_ = layer;
  ui->presetParamLabel->setText(tr("Parameter Settings") + "("+ layer->name() + ")");
  this->blockInputSignals();
  ui->powerSpinBox->setValue(layer->power());
  ui->speedSpinBox->setValue(layer->speed());
  ui->repeatSpinBox->setValue(layer->repeat());
  ui->backlashSpinBox->setValue(layer->xBacklash());
  ui->freqSpinBox->setValue(layer->frequency());
  ui->pulseWidthSpinBox->setValue(layer->pulseWidth());
  ui->fillIntervalSpinBox->setValue(layer->fillInterval());
  ui->fillAngleSpinBox->setValue(layer->fillAngle());
  ui->presetComboBox->setCurrentIndex(previous_index);
  this->unblockInputSignals();
  updateMovingComboBox();
  updateParams();
}

/*
  To avoid confusing with the recommended parameters,
  we change the preset combobox to `custom` whenever user changes the parameters.
*/
void LayerParamsPanel::setToCustom() {
  ui->presetComboBox->setCurrentIndex(ui->presetComboBox->count() - 2);
}

void LayerParamsPanel::setPresetIndex(int preset_index, int param_index) {
  PresetSettings* settings = &PresetSettings::getInstance();
  PresetSettings::Preset preset = settings->getTargetPreset(preset_index);
  if(param_index >= preset.params.size()) {
    qInfo() << Q_FUNC_INFO << " is wrong !!!";
    return;
  }
  ui->presetComboBox->blockSignals(true);
  ui->presetComboBox->clear();
  for (auto &param: preset.params) {
    ui->presetComboBox->addItem(param.name, param.toJson());
  }
  ui->presetComboBox->addItem(tr("Custom"));
  ui->presetComboBox->addItem(tr("More..."));
  ui->presetComboBox->blockSignals(false);
  if(param_index == -1) {
    ui->presetComboBox->blockSignals(true);
    ui->presetComboBox->setCurrentIndex(ui->presetComboBox->count() - 2);
    ui->presetComboBox->blockSignals(false);
  } else {
    PresetSettings::Param param = settings->getTargetParam(preset_index, param_index);
    this->blockInputSignals();
    ui->powerSpinBox->setValue(param.power);
    ui->speedSpinBox->setValue(param.speed);
    ui->repeatSpinBox->setValue(param.repeat);
    ui->presetComboBox->setCurrentIndex(param_index);
    this->unblockInputSignals();
    updateParams();
    Q_EMIT editLayerParams(current_params_);
  }
}

void LayerParamsPanel::setLayerParam(double strength, double speed, int repeat) {
  this->blockInputSignals();
  ui->powerSpinBox->setValue(strength);
  ui->speedSpinBox->setValue(speed);
  ui->repeatSpinBox->setValue(repeat);
  this->unblockInputSignals();
}

void LayerParamsPanel::setLayerBacklash(double backlash) {
  this->blockInputSignals();
  ui->backlashSpinBox->setValue(backlash);
  this->unblockInputSignals();
}

const LayerParameters LayerParamsPanel::updateParams() {
  current_params_.strength = ui->powerSpinBox->value();
  current_params_.speed = ui->speedSpinBox->value();
  current_params_.repeat = ui->repeatSpinBox->value();
  current_params_.backlash = ui->backlashSpinBox->value();
  current_params_.frequency = ui->freqSpinBox->value();
  current_params_.pulse_width = ui->pulseWidthSpinBox->value();
  current_params_.fill_interval = ui->fillIntervalSpinBox->value();
  current_params_.fill_angle = ui->fillAngleSpinBox->value();
  if (main_window_->selectedMachineParam().board_type == MachineSettings::MachineParam::BoardType::BSL_2024) {
    ui->fiberOptions->show();
  } else {
    ui->fiberOptions->hide();
  }
  return current_params_;
}

void LayerParamsPanel::setLayerParamLock(bool enable) {
  ui->presetComboBox->setEnabled(enable);
}