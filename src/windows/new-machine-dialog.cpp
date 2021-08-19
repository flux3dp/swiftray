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
  ui->machineList->clear();
  for (auto &machine : MachineSettings::database()) {
    QListWidgetItem *mach_item = new QListWidgetItem;
    mach_item->setData(Qt::UserRole, machine.toJson());
    mach_item->setText(machine.brand + " " + machine.name);
    mach_item->setIcon(machine.icon());
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
