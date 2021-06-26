#include <QDebug>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <settings/param-settings.h>
#include "preset-manager.h"
#include "ui_preset-manager.h"
#include <widgets/panels/layer-params-panel.h>

PresetManager::PresetManager(QWidget *parent) :
     QDialog(parent),
     layer_panel_((LayerParamsPanel *) parent),
     ui(new Ui::PresetManager) {
  ui->setupUi(this);
  setWindowTitle("Preset Manager");
  loadSettings();
  loadStyles();
  registerEvents();
  // TODO (Block moving default parameters when it's already in userlist)
}

void PresetManager::loadStyles() {}

void PresetManager::loadSettings() {
  ParamSettings settings;
  qInfo() << "Settings" << settings.toJson();
  ui->enabledList->clear();
  for (auto &param : settings.params_) {
    QListWidgetItem *param_item = new QListWidgetItem;
    param_item->setData(Qt::UserRole, param.toJson());
    param_item->setText(param.name);
    ui->enabledList->addItem(param_item);
  }
  // TODO (Access canvas here to get machine model)
  ParamSettings default_settings("beamo", true);
  qInfo() << "Settings" << default_settings.toJson();
  ui->defaultList->clear();
  for (auto &param : default_settings.params_) {
    QListWidgetItem *param_item = new QListWidgetItem;
    param_item->setData(Qt::UserRole, param.toJson());
    param_item->setText(param.name);
    ui->defaultList->addItem(param_item);
  }
}

void PresetManager::registerEvents() {
  connect(ui->addLayerBtn, &QAbstractButton::clicked, [=]() {
    QListWidgetItem *param_item = new QListWidgetItem;
    ParamSettings::ParamSet param;
    param.name = "New Custom Parameter";
    param_item->setData(Qt::UserRole, param.toJson());
    param_item->setText(param.name);
    ui->enabledList->addItem("New Custom Parameter");
  });
  connect(ui->enabledList, &QListWidget::currentItemChanged, [=](QListWidgetItem *item, QListWidgetItem *previous) {
    auto obj = item->data(Qt::UserRole).toJsonObject();
    auto param = ParamSettings::ParamSet::fromJson(obj);
    ui->editor->setEnabled(true);
    ui->paramTitle->setText(param.name);
    ui->speed->setValue(param.speed);
    ui->power->setValue(param.power);
    ui->repeat->setValue(param.repeat);
    ui->stepHeight->setValue(param.step_height);
  });

  connect(ui->defaultList, &QListWidget::currentItemChanged, [=](QListWidgetItem *item, QListWidgetItem *previous) {
    auto obj = item->data(Qt::UserRole).toJsonObject();
    auto param = ParamSettings::ParamSet::fromJson(obj);
    ui->editor->setEnabled(false);
    ui->paramTitle->setText(param.name);
    ui->speed->setValue(param.speed);
    ui->power->setValue(param.power);
    ui->repeat->setValue(param.repeat);
    ui->stepHeight->setValue(param.step_height);
  });
}

PresetManager::~PresetManager() {
  delete ui;
  qInfo() << "Dialog Destructed";
}
