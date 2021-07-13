#include <QPushButton>
#include "new-machine-dialog.h"
#include "ui_new-machine-dialog.h"
#include <settings/machine-settings.h>

NewMachineDialog::NewMachineDialog(QWidget *parent) :
     QDialog(parent),
     ui(new Ui::NewMachineDialog),
     BaseContainer() {
  ui->setupUi(this);
  ui->machineList->setIconSize(QSize(80, 80));
  initializeContainer();
}

void NewMachineDialog::registerEvents() {
  ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
  connect(ui->machineList, &QListWidget::currentItemChanged, [=]() {
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
  });
}

void NewMachineDialog::loadSettings() {
  MachineSettings mach_settings;
  QFile file(":/resources/machines.json");
  file.open(QFile::ReadOnly);
  mach_settings.loadJson(QJsonDocument::fromJson(file.readAll()).object());
  ui->machineList->clear();
  for (auto &machine : mach_settings.machines_) {
    QListWidgetItem *mach_item = new QListWidgetItem;
    mach_item->setData(Qt::UserRole, machine.toJson());
    mach_item->setText(machine.name);
    mach_item->setIcon(QIcon(machine.icon));
    ui->machineList->addItem(mach_item);
  }
}

MachineSettings::MachineSet NewMachineDialog::machine() const {
  auto json = ui->machineList->currentItem()->data(Qt::UserRole).toJsonObject();
  return MachineSettings::MachineSet::fromJson(json);
};

NewMachineDialog::~NewMachineDialog() {
  delete ui;
}
