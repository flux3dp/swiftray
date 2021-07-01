#include <QAction>
#include <QDebug>
#include <windows/new-machine-dialog.h>
#include <settings/machine-settings.h>
#include <QListWidgetItem>
#include "machine-manager.h"
#include "ui_machine-manager.h"

MachineManager::MachineManager(QWidget *parent) :
     QDialog(parent),
     ui(new Ui::MachineManager) {
  ui->setupUi(this);
  ui->machineList->setIconSize(QSize(32, 32));
  loadSettings();
  loadWidgets();
  loadStyles();
  registerEvents();
}

MachineManager::~MachineManager() {
  delete ui;
}

void MachineManager::loadSettings() {
  MachineSettings settings;
  qInfo() << "Settings" << settings.toJson();
  ui->machineList->clear();
  for (auto &machine : settings.machines_) {
    QListWidgetItem *param_item = new QListWidgetItem;
    param_item->setData(Qt::UserRole, machine.toJson());
    param_item->setText(machine.name);
    param_item->setIcon(QIcon(machine.icon));
    ui->machineList->addItem(param_item);
  }
}


void MachineManager::loadStyles() {
}

void MachineManager::loadWidgets() {
  ui->editorTabs->setEnabled(false);
}

void MachineManager::registerEvents() {
  connect(ui->addBtn, &QAbstractButton::clicked, [=]() {
    auto *dialog = new NewMachineDialog(this);
    if (dialog->exec() == 0) return;
    QListWidgetItem *machine_item = new QListWidgetItem;
    auto machine = dialog->machine();
    machine_item->setData(Qt::UserRole, machine.toJson());
    machine_item->setText(machine.name);
    machine_item->setIcon(QIcon(machine.icon));
    ui->machineList->addItem(machine_item);
    save();
  });

  connect(ui->removeBtn, &QAbstractButton::clicked, [=]() {
    if (ui->machineList->currentItem() != nullptr) {
      auto item = ui->machineList->currentItem();
      ui->machineList->takeItem(ui->machineList->row(item));
      save();
    }
  });

  connect(ui->machineList, &QListWidget::currentItemChanged, [=](QListWidgetItem *item, QListWidgetItem *previous) {
    auto obj = item->data(Qt::UserRole).toJsonObject();
    auto param = MachineSettings::MachineSet::fromJson(obj);
    ui->editorTabs->setEnabled(true);
    ui->nameLineEdit->setText(param.name);
    ui->modelComboBox->setCurrentText(param.model);
    ui->widthSpinBox->setValue(param.width);
    ui->heightSpinBox->setValue(param.height);
    ui->controllerComboBox->setCurrentIndex((int) param.board_type);
  });

  connect(ui->nameLineEdit, &QLineEdit::textChanged, [=](QString text) {
    auto item = ui->machineList->currentItem();
    item->setText(text);
    auto mach = MachineSettings::MachineSet::fromJson(item->data(Qt::UserRole).toJsonObject());
    mach.name = text;
    item->setData(Qt::UserRole, mach.toJson());
    save();
  });
};

void MachineManager::save() {
  MachineSettings settings;
  settings.machines_.clear();
  for (int i = 0; i < ui->machineList->count(); i++) {
    auto data = ui->machineList->item(i)->data(Qt::UserRole).toJsonObject();
    settings.machines_ << MachineSettings::MachineSet::fromJson(data);
  }
  settings.save();
}