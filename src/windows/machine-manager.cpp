#include <QAction>
#include <QDebug>
#include <windows/new-machine-dialog.h>
#include <settings/machine-settings.h>
#include <QListWidgetItem>
#include "machine-manager.h"
#include "ui_machine-manager.h"

MachineManager::MachineManager(QWidget *parent) :
     QDialog(parent),
     ui(new Ui::MachineManager),
     BaseContainer() {
  ui->setupUi(this);
  ui->machineList->
       setIconSize(QSize(32, 32)
  );
  initializeContainer();
}

MachineManager::~MachineManager() {
  delete ui;
}

void MachineManager::loadSettings() {
  MachineSettings settings;
  // If we call clear() when some item is selected, it lead to crash. Block signals here.
  ui->machineList->blockSignals(true);
  ui->machineList->clear();
  ui->machineList->blockSignals(false);
  for (auto &machine : settings.machines()) {
    QListWidgetItem *param_item = new QListWidgetItem;
    param_item->setData(Qt::UserRole, machine.toJson());
    param_item->setText(machine.name);
    param_item->setIcon(machine.icon());
    ui->machineList->addItem(param_item);
  }
}

void MachineManager::loadStyles() {
  ui->nameLineEdit->setStyleSheet("padding-left: 3px");
}

void MachineManager::loadWidgets() {
  ui->editorTabs->setEnabled(false);
}

void MachineManager::registerEvents() {
  connect(this, &QDialog::accepted, this, &MachineManager::save);

  connect(ui->addBtn, &QAbstractButton::clicked, [=]() {
    QListWidgetItem *machine_item = new QListWidgetItem;
    auto machine = MachineSettings::database();
    machine_item->setData(Qt::UserRole, machine[0].toJson());
    machine_item->setText(machine[0].name);
    machine_item->setIcon(machine[0].icon());
    ui->machineList->addItem(machine_item);
  });

  connect(ui->removeBtn, &QAbstractButton::clicked, [=]() {
    if (ui->machineList->currentItem() != nullptr) {
      auto item = ui->machineList->currentItem();
      ui->machineList->takeItem(ui->machineList->row(item));
    }
  });

  connect(ui->machineList, &QListWidget::currentItemChanged, [=](QListWidgetItem *item, QListWidgetItem *previous) {
    auto obj = item->data(Qt::UserRole).toJsonObject();
    auto mach = MachineSettings::MachineSet::fromJson(obj);
    ui->editorTabs->setEnabled(true);
    ui->nameLineEdit->setText(mach.name);
    ui->widthSpinBox->setValue(mach.width);
    ui->heightSpinBox->setValue(mach.height);
    ui->controllerComboBox->setCurrentIndex((int) mach.board_type);
    switch (mach.origin) {
      case MachineSettings::MachineSet::OriginType::RearLeft:
        ui->rearLeftRadioButton->setChecked(true);
        break;
      case MachineSettings::MachineSet::OriginType::RearRight:
        ui->rearRightRadioButton->setChecked(true);
        break;
      case MachineSettings::MachineSet::OriginType::FrontLeft:
        ui->frontLeftRadioButton->setChecked(true);
        break;
      case MachineSettings::MachineSet::OriginType::FrontRight:
        ui->frontRightRadioButton->setChecked(true);
        break;
    }
    connect(ui->rearLeftRadioButton, &QRadioButton::toggled, [=](bool checked) {
      if (checked) emit originChanged(MachineSettings::MachineSet::OriginType::RearLeft);
    });

    connect(ui->rearRightRadioButton, &QRadioButton::toggled, [=](bool checked) {
      if (checked) emit originChanged(MachineSettings::MachineSet::OriginType::RearRight);
    });

    connect(ui->frontLeftRadioButton, &QRadioButton::toggled, [=](bool checked) {
      if (checked) emit originChanged(MachineSettings::MachineSet::OriginType::FrontLeft);
    });

    connect(ui->frontRightRadioButton, &QRadioButton::toggled, [=](bool checked) {
      if (checked) emit originChanged(MachineSettings::MachineSet::OriginType::FrontRight);
    });
  });

  connect(ui->nameLineEdit, &QLineEdit::textChanged, [=](QString text) {
    auto item = ui->machineList->currentItem();
    item->setText(text);
    auto mach = MachineSettings::MachineSet::fromJson(item->data(Qt::UserRole).toJsonObject());
    mach.name = text;
    item->setData(Qt::UserRole, mach.toJson());
  });

  connect(ui->widthSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [=](int width) {
    auto item = ui->machineList->currentItem();
    auto mach = MachineSettings::MachineSet::fromJson(item->data(Qt::UserRole).toJsonObject());
    mach.width = width;
    item->setData(Qt::UserRole, mach.toJson());
  });

  connect(ui->heightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [=](int height) {
    auto item = ui->machineList->currentItem();
    auto mach = MachineSettings::MachineSet::fromJson(item->data(Qt::UserRole).toJsonObject());
    mach.height = height;
    item->setData(Qt::UserRole, mach.toJson());
  });

  connect(ui->controllerComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index) {
    auto item = ui->machineList->currentItem();
    auto mach = MachineSettings::MachineSet::fromJson(item->data(Qt::UserRole).toJsonObject());
    mach.board_type = (MachineSettings::MachineSet::BoardType) index;
    item->setData(Qt::UserRole, mach.toJson());
  });
};

void MachineManager::originChanged(MachineSettings::MachineSet::OriginType origin) {
  auto item = ui->machineList->currentItem();
  auto mach = MachineSettings::MachineSet::fromJson(item->data(Qt::UserRole).toJsonObject());
  mach.origin = origin;
  item->setData(Qt::UserRole, mach.toJson());
}

void MachineManager::save() {
  MachineSettings settings;
  settings.machines().clear();
  for (int i = 0; i < ui->machineList->count(); i++) {
    auto data = ui->machineList->item(i)->data(Qt::UserRole).toJsonObject();
    settings.machines() << MachineSettings::MachineSet::fromJson(data);
  }
  settings.save();
}

void MachineManager::show() {
  loadSettings();
  QDialog::show();
}
