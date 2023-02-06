#include <QDebug>
#include "preset-manager.h"
#include "ui_preset-manager.h"
#include <settings/preset-settings.h>

// General concept: store state in the ui, and finally save the settings from ui state
// If we generate ui from states, the cost is rather high

PresetManager::PresetManager(QWidget *parent, int preset_index) :
     QDialog(parent),
     ui(new Ui::PresetManager),
     BaseContainer() {
  ui->setupUi(this);
  initializeContainer();
  current_param_ = nullptr;
  ui->presetList->setCurrentRow(preset_index);
}

PresetManager::~PresetManager() {
  delete ui;
}

void PresetManager::loadStyles() {
  ui->groupBox->setTitle(" ");
}

void PresetManager::loadSettings() {
  PresetSettings* settings = &PresetSettings::getInstance();
  // qInfo() << "Settings" << settings->toJson();
  ui->presetList->clear();
  ui->paramList->clear();
  for (auto &preset : settings->presets()) {
    QListWidgetItem *item = new QListWidgetItem(preset.name);
    item->setData(Qt::UserRole, preset.toJson());
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    ui->presetList->addItem(item);
  }
}

void PresetManager::registerEvents() {
  connect(ui->addParamBtn, &QAbstractButton::clicked, [=]() {
    PresetSettings::Param param;
    auto item = new QListWidgetItem(param.name);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    item->setData(Qt::UserRole, param.toJson());
    ui->paramList->addItem(item);
    ui->paramList->scrollToBottom();
    ui->paramList->setCurrentRow(ui->paramList->count() - 1);
    ui->removeParamBtn->setEnabled(true);
    updatePresetData();
  });

  connect(ui->removeParamBtn, &QAbstractButton::clicked, [=]() {
    if(ui->paramList->currentItem() != nullptr) {
      auto item = ui->paramList->currentItem();
      ui->paramList->takeItem(ui->paramList->row(item));
      updatePresetData();
    }
  });

  connect(ui->addPresetBtn, &QAbstractButton::clicked, [=]() {
    PresetSettings::Preset preset;
    auto item = new QListWidgetItem(preset.name);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    item->setData(Qt::UserRole, preset.toJson());
    ui->presetList->addItem(item);
    ui->presetList->scrollToBottom();
    ui->presetList->setCurrentRow(ui->presetList->count() - 1);
    ui->removePresetBtn->setEnabled(true);
  });

  connect(ui->removePresetBtn, &QAbstractButton::clicked, [=]() {
    if(ui->presetList->currentItem() != nullptr) {
      auto item = ui->presetList->currentItem();
      ui->presetList->takeItem(ui->presetList->row(item));
    }
    if(ui->presetList->count() == 1) {
      ui->removePresetBtn->setEnabled(false);
    }
  });

  connect(ui->paramList, &QListWidget::currentItemChanged, [=](QListWidgetItem *item, QListWidgetItem *previous) {
    current_param_ = item;
    if (item == nullptr) {
      ui->speedSpinBox->setEnabled(false);
      ui->powerSpinBox->setEnabled(false);
      ui->repeatSpinBox->setEnabled(false);
      ui->removeParamBtn->setEnabled(false);
      return;
    }
    ui->speedSpinBox->setEnabled(true);
    ui->powerSpinBox->setEnabled(true);
    ui->repeatSpinBox->setEnabled(true);
    auto obj = item->data(Qt::UserRole).toJsonObject();
    auto param = PresetSettings::Param::fromJson(obj);
    ui->paramTitle->setText(param.name);
    ui->speedSpinBox->setValue(param.speed);
    ui->powerSpinBox->setValue(param.power);
    ui->repeatSpinBox->setValue(param.repeat);
    ui->stepHeightSpinBox->setValue(param.step_height);
  });

  connect(ui->presetList, &QListWidget::currentItemChanged, [=](QListWidgetItem *item, QListWidgetItem *previous) {
    if (item == nullptr) {
      ui->framingSpinBox->setEnabled(false);
      ui->pulseSpinBox->setEnabled(false);
      return;
    }
    ui->framingSpinBox->setEnabled(true);
    ui->pulseSpinBox->setEnabled(true);
    showPreset();
  });

  // Todo (Swap to itemDelegate https://stackoverflow.com/questions/22049129/qt-signal-for-when-qlistwidget-row-is-edited)
  connect(ui->presetList, &QListWidget::itemChanged, [=](QListWidgetItem *item) {
    if (item == nullptr) return;
    auto obj = item->data(Qt::UserRole).toJsonObject();
    auto preset = PresetSettings::Preset::fromJson(obj);
    if (item->text() != preset.name) {
      preset.name = item->text();
      item->setData(Qt::UserRole, preset.toJson());
      ui->presetTitle->setText(preset.name);
    }
  });

  connect(ui->paramList, &QListWidget::itemChanged, [=](QListWidgetItem *item) {
    current_param_ = item;
    if (item == nullptr) return;
    auto obj = item->data(Qt::UserRole).toJsonObject();
    auto param = PresetSettings::Param::fromJson(obj);
    if (item->text() != param.name) {
      param.name = item->text();
      item->setData(Qt::UserRole, param.toJson());
      ui->paramTitle->setText(param.name);
      updatePresetData();
    }
  });

  connect(ui->powerSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), [=](double value) {
    if(current_param_ != nullptr) {
      auto obj = current_param_->data(Qt::UserRole).toJsonObject();
      auto param = PresetSettings::Param::fromJson(obj);
      param.power = value;
      current_param_->setData(Qt::UserRole, param.toJson());
      updatePresetData();
    }
  });

  connect(ui->speedSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), [=](double value) {
    if(current_param_ != nullptr) {
      auto obj = current_param_->data(Qt::UserRole).toJsonObject();
      auto param = PresetSettings::Param::fromJson(obj);
      param.speed = value;
      current_param_->setData(Qt::UserRole, param.toJson());
      updatePresetData();
    }
  });

  connect(ui->repeatSpinBox, qOverload<int>(&QSpinBox::valueChanged), [=](int value) {
    if(current_param_ != nullptr) {
      auto obj = current_param_->data(Qt::UserRole).toJsonObject();
      auto param = PresetSettings::Param::fromJson(obj);
      param.repeat = value;
      current_param_->setData(Qt::UserRole, param.toJson());
      updatePresetData();
    }
  });

  connect(ui->framingSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), [=](double value) {
    auto item = ui->presetList->currentItem();
    if(item != nullptr) {
      auto obj = item->data(Qt::UserRole).toJsonObject();
      auto preset = PresetSettings::Preset::fromJson(obj);
      preset.framing_power = value;
      item->setFlags(item->flags() | Qt::ItemIsEditable);
      item->setData(Qt::UserRole, preset.toJson());
    }
  });

  connect(ui->pulseSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), [=](double value) {
    auto item = ui->presetList->currentItem();
    if(item != nullptr) {
      auto obj = item->data(Qt::UserRole).toJsonObject();
      auto preset = PresetSettings::Preset::fromJson(obj);
      preset.pulse_power = value;
      item->setFlags(item->flags() | Qt::ItemIsEditable);
      item->setData(Qt::UserRole, preset.toJson());
    }
  });
  
  connect(ui->resetButton, &QAbstractButton::clicked, [=](bool checked) {
    PresetSettings* settings = &PresetSettings::getInstance();
    ui->presetList->clear();
    ui->paramList->clear();
    for (auto &preset : settings->getOriginPreset()) {
      QListWidgetItem *item = new QListWidgetItem(preset.name);
      item->setData(Qt::UserRole, preset.toJson());
      item->setFlags(item->flags() | Qt::ItemIsEditable);
      ui->presetList->addItem(item);
    }
    ui->presetList->setCurrentRow(0);
  });

  connect(ui->copyButton, &QAbstractButton::clicked, [=](bool checked) {
    auto item = ui->presetList->currentItem();
    if(item != nullptr) {
      QJsonObject new_obj(item->data(Qt::UserRole).toJsonObject());
      auto preset = PresetSettings::Preset::fromJson(new_obj);
      preset.name += "-copy";
      auto new_item = new QListWidgetItem(preset.name);
      new_item->setFlags(new_item->flags() | Qt::ItemIsEditable);
      new_item->setData(Qt::UserRole, preset.toJson());
      ui->presetList->addItem(new_item);
      ui->presetList->scrollToBottom();
      ui->presetList->setCurrentRow(ui->presetList->count() - 1);
    }
  });

  connect(ui->buttonBox, &QDialogButtonBox::accepted, [=]() {
    save();
    if(current_param_ != nullptr) {
      Q_EMIT updateCurrentIndex(ui->presetList->currentRow(), ui->paramList->currentRow());
    } else {
      Q_EMIT updateCurrentPresetIndex(ui->presetList->currentRow());
    }
  });
}

void PresetManager::updatePresetData() {
  PresetSettings::Preset p;
  for (int i = 0; i < ui->paramList->count(); i++) {
    auto data = ui->paramList->item(i)->data(Qt::UserRole).toJsonObject();
    p.params << PresetSettings::Param::fromJson(data);
  }
  if(ui->paramList->count() == 0) {
    ui->removeParamBtn->setEnabled(false);
  }
  ui->presetList->currentItem()->setData(Qt::UserRole, p.toJson());
}

void PresetManager::save() {
  PresetSettings* settings = &PresetSettings::getInstance();
  QList<PresetSettings::Preset> presets;
  for (int i = 0; i < ui->presetList->count(); i++) {
    auto data = ui->presetList->item(i)->data(Qt::UserRole).toJsonObject();
    presets << PresetSettings::Preset::fromJson(data);
  }
  settings->setPresets(presets);
  settings->save();
}

void PresetManager::showPreset() {
  auto item = ui->presetList->currentItem();
  auto obj = item->data(Qt::UserRole).toJsonObject();
  auto preset = PresetSettings::Preset::fromJson(obj);
  ui->paramList->clear();
  for (auto &param : preset.params) {
    QListWidgetItem *item = new QListWidgetItem(param.name);
    item->setData(Qt::UserRole, param.toJson());
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    ui->paramList->addItem(item);
  }
  if(ui->paramList->count() == 0 || current_param_ == nullptr) {
    ui->removeParamBtn->setEnabled(false);
  } else {
    ui->removeParamBtn->setEnabled(true);
  }
  ui->presetTitle->setText(preset.name);
  ui->framingSpinBox->setValue(preset.framing_power);
  ui->pulseSpinBox->setValue(preset.pulse_power);
}
