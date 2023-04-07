#include <QAction>
#include <QDebug>
#include <QListWidgetItem>
#include <windows/mainwindow.h>
#include <settings/machine-settings.h>
#include "machine-manager.h"
#include "ui_machine-manager.h"
#include <windows/welcome-dialog.h>

MachineManager::MachineManager(QWidget *parent, int machine_index) :
     QDialog(parent),
     ui(new Ui::MachineManager),
     BaseContainer() {
  ui->setupUi(this);
  ui->machineList->setIconSize(QSize(32, 32));
  initializeContainer();
  ui->machineList->setCurrentRow(machine_index);
  axis_group_ = new QButtonGroup(this);
  axis_group_->addButton(ui->YRadioButton);
  axis_group_->addButton(ui->ZRadioButton);
  axis_group_->addButton(ui->ARadioButton);
}

MachineManager::~MachineManager() {
  delete ui;
}

void MachineManager::loadSettings() {
  MachineSettings* settings = &MachineSettings::getInstance();
  ui->machineList->blockSignals(true);
  ui->machineList->clear();
  for (const auto &machine : settings->getMachines()) {
    QListWidgetItem *param_item = new QListWidgetItem;
    param_item->setData(Qt::UserRole, machine.toJson());
    param_item->setText(machine.name);
    param_item->setIcon(machine.icon());
    ui->machineList->addItem(param_item);
  }
  if(ui->machineList->count() == 1) {
    ui->removeBtn->setEnabled(false);
  } else {
    ui->removeBtn->setEnabled(true);
  }
  ui->machineList->blockSignals(false);

  // TODO: Create the machines based on settings
  //       machine list push back ...
  //       set one from machine list as active machine
}

void MachineManager::loadStyles() {
  ui->nameLineEdit->setStyleSheet("padding-left: 3px");
}

void MachineManager::loadWidgets() {
  ui->frame->setEnabled(false);
}

void MachineManager::registerEvents() {
  connect(this, &QDialog::accepted, this, &MachineManager::save);

  connect(ui->addBtn, &QAbstractButton::clicked, [=]() {
    ui->addBtn->setEnabled(false);
    WelcomeDialog* machine_setup = new WelcomeDialog(this);
    machine_setup->setupMachine();
    connect(machine_setup, &WelcomeDialog::addNewMachine, [=](MachineSettings::MachineParam new_param) {
      auto item = new QListWidgetItem(new_param.name);
      item->setFlags(item->flags() | Qt::ItemIsEditable);
      item->setData(Qt::UserRole, new_param.toJson());
      ui->machineList->addItem(item);
      ui->machineList->scrollToBottom();
      ui->machineList->setCurrentRow(ui->machineList->count() - 1);
      ui->removeBtn->setEnabled(true);
    });
    connect(machine_setup, &WelcomeDialog::finished, [=](int result) {
      ui->addBtn->setEnabled(true);
    });
    machine_setup->show();
    machine_setup->activateWindow();
    machine_setup->raise();
  });

  connect(ui->removeBtn, &QAbstractButton::clicked, [=]() {
    if (ui->machineList->currentItem() != nullptr) {
      auto item = ui->machineList->currentItem();
      ui->machineList->takeItem(ui->machineList->row(item));
    }
    if(ui->machineList->count() == 1) {
      ui->removeBtn->setEnabled(false);
    }
  });

  connect(ui->machineList, &QListWidget::currentItemChanged, [=](QListWidgetItem *item, QListWidgetItem *previous) {
    auto obj = item->data(Qt::UserRole).toJsonObject();
    auto mach = MachineSettings::MachineParam::fromJson(obj);
    ui->frame->setEnabled(true);
    ui->nameLineEdit->setText(mach.name);
    ui->widthSpinBox->setValue(mach.width);
    ui->heightSpinBox->setValue(mach.height);
    ui->controllerComboBox->setCurrentIndex((int) mach.board_type);
    ui->speedSpinBox->setValue(mach.travel_speed);
    switch (mach.origin) {
      case MachineSettings::MachineParam::OriginType::RearLeft:
        ui->rearLeftRadioButton->setChecked(true);
        break;
      case MachineSettings::MachineParam::OriginType::RearRight:
        ui->rearRightRadioButton->setChecked(true);
        break;
      case MachineSettings::MachineParam::OriginType::FrontLeft:
        ui->frontLeftRadioButton->setChecked(true);
        break;
      case MachineSettings::MachineParam::OriginType::FrontRight:
        ui->frontRightRadioButton->setChecked(true);
        break;
    }
    switch (mach.rotary_axis) {
      case 'Y':
        ui->YRadioButton->blockSignals(true);
        ui->YRadioButton->setChecked(true);
        ui->YRadioButton->blockSignals(false);
        break;
      case 'Z':
        ui->ZRadioButton->blockSignals(true);
        ui->ZRadioButton->setChecked(true);
        ui->ZRadioButton->blockSignals(false);
        break;
      case 'A':
        ui->ARadioButton->blockSignals(true);
        ui->ARadioButton->setChecked(true);
        ui->ARadioButton->blockSignals(false);
        break;
    }
    connect(ui->rearLeftRadioButton, &QRadioButton::toggled, [=](bool checked) {
      if (checked) originChanged(MachineSettings::MachineParam::OriginType::RearLeft);
    });
    connect(ui->rearRightRadioButton, &QRadioButton::toggled, [=](bool checked) {
      if (checked) originChanged(MachineSettings::MachineParam::OriginType::RearRight);
    });
    connect(ui->frontLeftRadioButton, &QRadioButton::toggled, [=](bool checked) {
      if (checked) originChanged(MachineSettings::MachineParam::OriginType::FrontLeft);
    });
    connect(ui->frontRightRadioButton, &QRadioButton::toggled, [=](bool checked) {
      if (checked) originChanged(MachineSettings::MachineParam::OriginType::FrontRight);
    });
  });

  connect(ui->nameLineEdit, &QLineEdit::textChanged, [=](QString text) {
    auto item = ui->machineList->currentItem();
    item->setText(text);
    auto mach = MachineSettings::MachineParam::fromJson(item->data(Qt::UserRole).toJsonObject());
    mach.name = text;
    std::size_t found = mach.name.toStdString().find("Lazervida");
    if(found!=std::string::npos) {
      mach.is_high_speed_mode = true;
    }
    else {
      mach.is_high_speed_mode = false;
    }
    item->setData(Qt::UserRole, mach.toJson());
  });

  connect(ui->widthSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [=](int width) {
    auto item = ui->machineList->currentItem();
    auto mach = MachineSettings::MachineParam::fromJson(item->data(Qt::UserRole).toJsonObject());
    mach.width = width;
    item->setData(Qt::UserRole, mach.toJson());
  });

  connect(ui->heightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [=](int height) {
    auto item = ui->machineList->currentItem();
    auto mach = MachineSettings::MachineParam::fromJson(item->data(Qt::UserRole).toJsonObject());
    mach.height = height;
    item->setData(Qt::UserRole, mach.toJson());
  });

  connect(ui->controllerComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index) {
    auto item = ui->machineList->currentItem();
    auto mach = MachineSettings::MachineParam::fromJson(item->data(Qt::UserRole).toJsonObject());
    mach.board_type = (MachineSettings::MachineParam::BoardType) index;
    item->setData(Qt::UserRole, mach.toJson());
  });

  connect(ui->speedSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), [=](double value) {
    auto item = ui->machineList->currentItem();
    auto mach = MachineSettings::MachineParam::fromJson(item->data(Qt::UserRole).toJsonObject());
    mach.travel_speed = value;
    item->setData(Qt::UserRole, mach.toJson());
  });

  connect(ui->YRadioButton, &QRadioButton::toggled, [=](bool checked) {
    if (checked) {
      auto item = ui->machineList->currentItem();
      auto mach = MachineSettings::MachineParam::fromJson(item->data(Qt::UserRole).toJsonObject());
      mach.rotary_axis = 'Y';
      item->setData(Qt::UserRole, mach.toJson());
    }
  });

  connect(ui->ZRadioButton, &QRadioButton::toggled, [=](bool checked) {
    if (checked) {
      auto item = ui->machineList->currentItem();
      auto mach = MachineSettings::MachineParam::fromJson(item->data(Qt::UserRole).toJsonObject());
      mach.rotary_axis = 'Z';
      item->setData(Qt::UserRole, mach.toJson());
    }
  });

  connect(ui->ARadioButton, &QRadioButton::toggled, [=](bool checked) {
    if (checked) {
      auto item = ui->machineList->currentItem();
      auto mach = MachineSettings::MachineParam::fromJson(item->data(Qt::UserRole).toJsonObject());
      mach.rotary_axis = 'A';
      item->setData(Qt::UserRole, mach.toJson());
    }
  });
};

void MachineManager::setMachineIndex(int index) {
  loadSettings();
  ui->machineList->setCurrentRow(index);
}

void MachineManager::setRotaryAxis(char rotary_axis) {
  switch (rotary_axis) {
    case 'Y':
      ui->YRadioButton->blockSignals(true);
      ui->YRadioButton->setChecked(true);
      ui->YRadioButton->blockSignals(false);
      break;
    case 'Z':
      ui->ZRadioButton->blockSignals(true);
      ui->ZRadioButton->setChecked(true);
      ui->ZRadioButton->blockSignals(false);
      break;
    case 'A':
      ui->ARadioButton->blockSignals(true);
      ui->ARadioButton->setChecked(true);
      ui->ARadioButton->blockSignals(false);
      break;
    default:
      qInfo() << Q_FUNC_INFO << " get axis: " << rotary_axis << " over range";
      break;
  }
}

void MachineManager::originChanged(MachineSettings::MachineParam::OriginType origin) {
  auto item = ui->machineList->currentItem();
  auto mach = MachineSettings::MachineParam::fromJson(item->data(Qt::UserRole).toJsonObject());
  mach.origin = origin;
  item->setData(Qt::UserRole, mach.toJson());
}

void MachineManager::save() {
  MachineSettings* settings = &MachineSettings::getInstance();
  settings->clearMachines();
  for (int i = 0; i < ui->machineList->count(); i++) {
    auto data = ui->machineList->item(i)->data(Qt::UserRole).toJsonObject();
    settings->addMachine(MachineSettings::MachineParam::fromJson(data));
  }
  settings->save();
  if (ui->machineList->currentItem() != nullptr) {
    Q_EMIT updateCurrentMachineIndex(ui->machineList->currentRow());
  }
}

void MachineManager::show() {
  // loadSettings();
  QDialog::show();
}
