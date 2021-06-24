#include <QDebug>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <settings/param-settings.h>
#include "preset-manager.h"
#include "ui_preset-manager.h"

PresetManager::PresetManager(QWidget *parent) :
     QDialog(parent),
     ui(new Ui::PresetManager) {
  ui->setupUi(this);
  setWindowTitle("Preset Manager");
  loadSettings();
  loadStyles();
  registerEvents();
}

void PresetManager::loadStyles() {}

void PresetManager::loadSettings() {
  ParamSettings settings;
  qInfo() << "Settings" << settings.toJson();
}

void PresetManager::registerEvents() {
  connect(ui->addLayerBtn, &QAbstractButton::clicked, [=]() {
    ui->enabledList->addItem("New Custom Parameter");
  });
}

PresetManager::~PresetManager() {
  delete ui;
  qInfo() << "Dialog Destructed";
}
