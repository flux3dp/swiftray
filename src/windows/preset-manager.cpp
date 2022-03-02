#include <QDebug>
#include <QSettings>
#include <QJsonObject>
#include "preset-manager.h"
#include "ui_preset-manager.h"
#include <widgets/panels/layer-params-panel.h>

// General concept: store state in the ui, and finally save the settings from ui state
// If we generate ui from states, the cost is rather high

PresetManager::PresetManager(QWidget *parent) :
     QDialog(parent),
     layer_panel_((LayerParamsPanel *) parent),
     ui(new Ui::PresetManager),
     BaseContainer() {
  ui->setupUi(this);
  setWindowTitle("Preset Manager");
  initializeContainer();
}

PresetManager::~PresetManager() {
  delete ui;
}

void PresetManager::loadStyles() {
  ui->groupBox->setTitle(" ");
}

void PresetManager::loadSettings() {
  PresetSettings* settings = &PresetSettings::getInstance();
  qInfo() << "Settings" << settings->toJson();
  ui->addParamBtn->hide();
  ui->removeParamBtn->hide();
  ui->addPresetBtn->hide();
  ui->removePresetBtn->hide();
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
    param.name = "New Custom Parameter";
    auto item = new QListWidgetItem(param.name);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    item->setData(Qt::UserRole, param.toJson());
    ui->paramList->addItem(item);
    ui->paramList->scrollToBottom();
    ui->paramList->setCurrentRow(ui->paramList->count() - 1);
    updatePresetData();
  });

  connect(ui->removeParamBtn, &QAbstractButton::clicked, [=]() {
    qInfo() << "Remove param" << ui->paramList->currentItem();
    auto item = ui->paramList->currentItem();
    ui->paramList->takeItem(ui->paramList->row(item));
    updatePresetData();
  });

  connect(ui->addPresetBtn, &QAbstractButton::clicked, [=]() {
    PresetSettings::Preset preset;
    preset.name = "New Preset";
    auto item = new QListWidgetItem(preset.name);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    item->setData(Qt::UserRole, preset.toJson());
    ui->presetList->addItem(item);
    ui->presetList->scrollToBottom();
    ui->presetList->setCurrentRow(ui->presetList->count() - 1);
  });

  connect(ui->removePresetBtn, &QAbstractButton::clicked, [=]() {
    auto item = ui->presetList->currentItem();
    ui->presetList->takeItem(ui->presetList->row(item));
  });

  connect(ui->paramList, &QListWidget::currentItemChanged, [=](QListWidgetItem *item, QListWidgetItem *previous) {
    if (item == nullptr) return;
    auto obj = item->data(Qt::UserRole).toJsonObject();
    auto param = PresetSettings::Param::fromJson(obj);
    ui->editor->setEnabled(true);
    ui->paramTitle->setText(param.name);
    ui->speedSpinBox->setValue(param.speed);
    ui->powerSpinBox->setValue(param.power);
    ui->repeatSpinBox->setValue(param.repeat);
    ui->stepHeightSpinBox->setValue(param.step_height);
  });

  connect(ui->presetList, &QListWidget::currentItemChanged, [=](QListWidgetItem *item, QListWidgetItem *previous) {
    if (item == nullptr) return;
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
    }
  });

  connect(ui->paramList, &QListWidget::itemChanged, [=](QListWidgetItem *item) {
    if (item == nullptr) return;
    auto obj = item->data(Qt::UserRole).toJsonObject();
    auto param = PresetSettings::Param::fromJson(obj);
    if (item->text() != param.name) {
      param.name = item->text();
      item->setData(Qt::UserRole, param.toJson());
      updatePresetData();
    }
  });
}

void PresetManager::updatePresetData() {
  PresetSettings::Preset p;
  for (int i = 0; i < ui->paramList->count(); i++) {
    auto data = ui->paramList->item(i)->data(Qt::UserRole).toJsonObject();
    p.params << PresetSettings::Param::fromJson(data);
  }
  ui->presetList->currentItem()->setData(Qt::UserRole, p.toJson());
}

void PresetManager::save() {
  PresetSettings* settings = &PresetSettings::getInstance();
  settings->presets_.clear();
  for (int i = 0; i < ui->presetList->count(); i++) {
    auto data = ui->presetList->item(i)->data(Qt::UserRole).toJsonObject();
    settings->presets_ << PresetSettings::Preset::fromJson(data);
  }
  settings->setCurrentIndex(ui->presetList->currentRow());
  settings->save();
  qInfo() << "Saving" << settings->toJson();
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
}
