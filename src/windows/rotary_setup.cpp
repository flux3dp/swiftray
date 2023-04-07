#include "rotary_setup.h"
#include "ui_rotary_setup.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <QMessageBox>
#include <windows/osxwindow.h>

#include <QDebug>

RotarySetup::RotarySetup(RotarySetting setting, QWidget *parent) :
  QDialog(parent),
  ui(new Ui::RotarySetup)
{
  ui->setupUi(this);
  axis_group_ = new QButtonGroup(this);
  axis_group_->addButton(ui->YRadioButton);
  axis_group_->addButton(ui->ZRadioButton);
  axis_group_->addButton(ui->ARadioButton);
  RotarySettings* settings = &RotarySettings::getInstance();
  rotary_list_.clear();
  for (auto &rotary : settings->rotarys()) {
    rotary_list_ << rotary;
  }
  ui->rollerButton->setIcon(QIcon(isDarkMode() ? ":/resources/images/dark/icon-rotary.png" : ":/resources/images/icon-rotary.png"));
  ui->chuckButton->setIcon(QIcon(isDarkMode() ? ":/resources/images/dark/icon-rotary-2.png" : ":/resources/images/icon-rotary-2.png"));
  if(isDarkMode()) {
    ui->rollerButton->setStyleSheet(selected_dark_);
    ui->chuckButton->setStyleSheet(unselect_dark_);
  } else {
    ui->rollerButton->setStyleSheet(selected_light_);
    ui->chuckButton->setStyleSheet(unselect_light_);
  }
  updateUI(setting);

  connect(ui->testBtn, &QAbstractButton::clicked, this, &RotarySetup::testRotary);
  connect(ui->rotaryCheckBox, &QCheckBox::stateChanged, [=](int state){
    if(state) {
      ui->mirrorCheckBox->setEnabled(true);
      ui->testBtn->setEnabled(true);
    }
    else {
      ui->mirrorCheckBox->setEnabled(false);
      ui->testBtn->setEnabled(false);
    }
    Q_EMIT rotaryModeChanged(state);
  });
  connect(ui->rollerButton, &QAbstractButton::clicked, [=](bool checked) {
    rotary_list_[ui->deviceComboBox->currentIndex()].rotary_type = RotarySettings::RotaryType::Roller;
    updateRotaryTypeSelect(RotarySettings::RotaryType::Roller);
  });
  connect(ui->chuckButton, &QAbstractButton::clicked, [=](bool checked) {
    rotary_list_[ui->deviceComboBox->currentIndex()].rotary_type = RotarySettings::RotaryType::Chuck;
    updateRotaryTypeSelect(RotarySettings::RotaryType::Chuck);
  });
  connect(ui->YRadioButton, &QRadioButton::toggled, [=](bool checked) {
    Q_EMIT rotaryAxisChanged('Y');
  });
  connect(ui->ZRadioButton, &QRadioButton::toggled, [=](bool checked) {
    Q_EMIT rotaryAxisChanged('Z');
  });
  connect(ui->ARadioButton, &QRadioButton::toggled, [=](bool checked) {
    Q_EMIT rotaryAxisChanged('A');
  });
  connect(ui->CircumSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), [=](double circumference) {
    ui->ObjectSpinBox->blockSignals(true);
    ui->ObjectSpinBox->setValue(circumference / M_PI);
    ui->ObjectSpinBox->blockSignals(false);
  });
  connect(ui->ObjectSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), [=](double diameter) {
    ui->CircumSpinBox->blockSignals(true);
    ui->CircumSpinBox->setValue(diameter * M_PI);
    ui->CircumSpinBox->blockSignals(false);
  });
  connect(ui->mmPerRotationSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), [=](double value) {
    rotary_list_[ui->deviceComboBox->currentIndex()].mm_per_rotation = value;
  });
  connect(ui->rollerDiameterSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), [=](double value) {
    rotary_list_[ui->deviceComboBox->currentIndex()].roller_diameter = value;
  });
  connect(ui->speedSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), [=](double speed) {
    rotary_list_[ui->deviceComboBox->currentIndex()].travel_speed = speed;
    Q_EMIT updateRotarySpeed(speed);
  });
  connect(ui->deviceComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index) {
    if(index == rotary_list_.size()) {
      RotarySettings::RotaryParam new_param;
      rotary_list_ << new_param;
      resetDeviceList(rotary_list_);
    }
    updateRotarySelect(index);
  });
  connect(ui->deviceNameEdit, &QLineEdit::textChanged, [=](QString text) {
    rotary_list_[ui->deviceComboBox->currentIndex()].name = text;
    int current_index = ui->deviceComboBox->currentIndex();
    resetDeviceList(rotary_list_);
    ui->deviceComboBox->blockSignals(true);
    ui->deviceComboBox->setCurrentIndex(current_index);
    ui->deviceComboBox->blockSignals(false);
    ui->deleteButton->setEnabled(true);
  });
  connect(ui->deleteButton, &QAbstractButton::clicked, [=]() {
    int current_index = ui->deviceComboBox->currentIndex();
    QMessageBox msgBox;
    msgBox.setText(tr("Do you want to delete current rotary?"));
    msgBox.addButton(tr("Yes"), QMessageBox::AcceptRole);
    msgBox.addButton(tr("Cancel"), QMessageBox::RejectRole);
    int ret = msgBox.exec();
    switch (ret) {
      case QMessageBox::AcceptRole:
        rotary_list_.removeAt(current_index);
        if(current_index > 0) current_index--;
        resetDeviceList(rotary_list_);
        ui->deviceComboBox->blockSignals(true);
        ui->deviceComboBox->setCurrentIndex(current_index);
        ui->deviceComboBox->blockSignals(false);
        updateRotarySelect(current_index);
        break;
      case QMessageBox::DestructiveRole:
          // Cancel was clicked
      default:
          // should never be reached
          break;
    }
  });

  connect(ui->buttonBox, &QDialogButtonBox::accepted, [=]() {
    char rotary_axis;
    if(ui->YRadioButton->isChecked()) {
      rotary_axis = 'Y';
    } else if(ui->ZRadioButton->isChecked()) {
      rotary_axis = 'Z';
    } else if(ui->ARadioButton->isChecked()) {
      rotary_axis = 'A';
    }
    Q_EMIT mirrorModeChanged(ui->mirrorCheckBox->checkState());
    Q_EMIT updateCircumference(ui->CircumSpinBox->value());

    RotarySettings* settings = &RotarySettings::getInstance();
    settings->setRotarys(rotary_list_);
    settings->save();
    Q_EMIT updateRotaryIndex(ui->deviceComboBox->currentIndex());
  });
  connect(ui->buttonBox, &QDialogButtonBox::rejected, [=]() {
    //rotary_axis & speed to reset
  });
}

RotarySetup::~RotarySetup() {
  delete ui;
}

void RotarySetup::setRotaryMode(bool is_rotary_mode) {
  ui->rotaryCheckBox->blockSignals(true);
  if(is_rotary_mode) {
    ui->rotaryCheckBox->setCheckState(Qt::Checked);
    ui->mirrorCheckBox->setEnabled(true);
    ui->testBtn->setEnabled(true);
  }
  else {
    ui->rotaryCheckBox->setCheckState(Qt::Unchecked);
    ui->mirrorCheckBox->setEnabled(false);
    ui->testBtn->setEnabled(false);
  }
  ui->rotaryCheckBox->blockSignals(false);
}

void RotarySetup::setMirrorMode(bool is_mirror_mode) {
  ui->mirrorCheckBox->blockSignals(true);
  if(is_mirror_mode) {
    ui->mirrorCheckBox->setCheckState(Qt::Checked);
  } else {
    ui->mirrorCheckBox->setCheckState(Qt::Unchecked);
  }
  ui->mirrorCheckBox->blockSignals(false);
}

void RotarySetup::setCircumference(double circumference) {
  ui->ObjectSpinBox->blockSignals(true);
  ui->CircumSpinBox->blockSignals(true);
  ui->ObjectSpinBox->setValue(circumference / M_PI);
  ui->CircumSpinBox->setValue(circumference);
  ui->ObjectSpinBox->blockSignals(false);
  ui->CircumSpinBox->blockSignals(false);
}

void RotarySetup::setRotaryAxis(char rotary_axis) {
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

/**
 * @brief Go through a rectangular path with its height aligned with rotary axis
 * 
 */
void RotarySetup::testRotary() {
  Q_EMIT actionTestRotary(ui->mmPerRotationSpinBox->value());
}

void RotarySetup::resetDeviceList(const QList<RotarySettings::RotaryParam> &rotary_list) {
  ui->deviceComboBox->blockSignals(true);
  ui->deviceComboBox->clear();
  for(auto &rotary : rotary_list) {
    if (rotary.name.isEmpty()) continue;
    ui->deviceComboBox->addItem(rotary.name);
  }
  ui->deviceComboBox->addItem("Add New Rotary");
  ui->deviceComboBox->blockSignals(false);
  if(ui->deviceComboBox->count() > 2) {
    ui->deleteButton->setEnabled(true);
  } else {
    ui->deleteButton->setEnabled(false);
  }
}

void RotarySetup::setRotaryIndex(int rotary_index) {
  RotarySettings* settings = &RotarySettings::getInstance();
  rotary_list_.clear();
  resetDeviceList(settings->rotarys());
  for(auto &rotary : settings->rotarys()) {
    rotary_list_ << rotary;
  }
  updateRotarySelect(rotary_index);
}

void RotarySetup::updateUI(RotarySetting setting) {
  ui->rotaryCheckBox->blockSignals(true);
  ui->mirrorCheckBox->blockSignals(true);
  if(setting.rotary_mode) {
    ui->rotaryCheckBox->setCheckState(Qt::Checked);
    ui->testBtn->setEnabled(true);
    ui->mirrorCheckBox->setEnabled(true);
  } else {
    ui->rotaryCheckBox->setCheckState(Qt::Unchecked);
    ui->testBtn->setEnabled(false);
    ui->mirrorCheckBox->setEnabled(false);
  }
  if(setting.mirror_mode) {
    ui->mirrorCheckBox->setCheckState(Qt::Checked);
  } else {
    ui->mirrorCheckBox->setCheckState(Qt::Unchecked);
  }
  ui->rotaryCheckBox->blockSignals(false);
  ui->mirrorCheckBox->blockSignals(false);
  switch(setting.rotary_axis) {
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
  ui->ObjectSpinBox->setValue(setting.circumference / M_PI);
  ui->CircumSpinBox->setValue(setting.circumference);
  setRotaryIndex(setting.rotary_index);
}

void RotarySetup::updateRotarySelect(int rotary_index) {
  RotarySettings::RotaryParam current_rotary = rotary_list_[rotary_index];
  updateRotaryTypeSelect(current_rotary.rotary_type);
  ui->deviceComboBox->blockSignals(true);
  ui->deviceNameEdit->blockSignals(true);
  ui->mmPerRotationSpinBox->blockSignals(true);
  ui->rollerDiameterSpinBox->blockSignals(true);
  ui->speedSpinBox->blockSignals(true);
  ui->deviceComboBox->setCurrentIndex(rotary_index);
  ui->deviceNameEdit->setText(current_rotary.name);
  ui->mmPerRotationSpinBox->setValue(current_rotary.mm_per_rotation);
  ui->rollerDiameterSpinBox->setValue(current_rotary.roller_diameter);
  ui->speedSpinBox->setValue(current_rotary.travel_speed);
  ui->deviceComboBox->blockSignals(false);
  ui->deviceNameEdit->blockSignals(false);
  ui->mmPerRotationSpinBox->blockSignals(false);
  ui->rollerDiameterSpinBox->blockSignals(false);
  ui->speedSpinBox->blockSignals(false);
  Q_EMIT updateRotarySpeed(current_rotary.travel_speed);
}

void RotarySetup::updateRotaryTypeSelect(RotarySettings::RotaryType rotary_type) {
  ui->rollerButton->blockSignals(true);
  ui->chuckButton->blockSignals(true);
  switch(rotary_type) {
    case RotarySettings::RotaryType::Roller:
      ui->rollerButton->setChecked(true);
      ui->chuckButton->setChecked(false);
      ui->rollerDiameterSpinBox->setEnabled(true);
      if(isDarkMode()) {
        ui->rollerButton->setStyleSheet(selected_dark_);
        ui->chuckButton->setStyleSheet(unselect_dark_);
      } else {
        ui->rollerButton->setStyleSheet(selected_light_);
        ui->chuckButton->setStyleSheet(unselect_light_);
      }
      break;
    case RotarySettings::RotaryType::Chuck:
      ui->chuckButton->setChecked(true);
      ui->rollerButton->setChecked(false);
      ui->rollerDiameterSpinBox->setEnabled(false);
      if(isDarkMode()) {
        ui->rollerButton->setStyleSheet(unselect_dark_);
        ui->chuckButton->setStyleSheet(selected_dark_);
      } else {
        ui->rollerButton->setStyleSheet(unselect_light_);
        ui->chuckButton->setStyleSheet(selected_light_);
      }
      break;
    default:
      break;
  }
  ui->rollerButton->blockSignals(false);
  ui->chuckButton->blockSignals(false);
}
