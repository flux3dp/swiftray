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

void PresetManager::loadStyles() {
  ui->groupBox->setTitle(" ");
}

void PresetManager::loadSettings() {
  ParamSettings settings;
  qInfo() << "Settings" << settings.toJson();
  ui->enabledList->clear();
  for (auto &param : settings.params_) {
    QListWidgetItem *item = new QListWidgetItem(param.name);
    item->setData(Qt::UserRole, param.toJson());
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    ui->enabledList->addItem(item);
  }
}

void PresetManager::registerEvents() {
  connect(ui->addParamBtn, &QAbstractButton::clicked, [=]() {
    ParamSettings::ParamSet param;
    param.name = "New Custom Parameter";
    QListWidgetItem *item = new QListWidgetItem(param.name);
    item->setData(Qt::UserRole, param.toJson());
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    ui->enabledList->addItem(item);
    ui->enabledList->setCurrentItem(item);
    ui->enabledList->scrollToItem(item, QAbstractItemView::PositionAtCenter);
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
}

PresetManager::~PresetManager() {
  delete ui;
  qInfo() << "Dialog Destructed";
}
