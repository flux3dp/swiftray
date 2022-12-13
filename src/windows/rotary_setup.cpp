#include "rotary_setup.h"
#include "ui_rotary_setup.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <QSettings>
#include <windows/osxwindow.h>

#include <QDebug>

RotarySetup::RotarySetup(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RotarySetup)
{
    ui->setupUi(this);
    axis_group_ = new QButtonGroup(this);
    ui->label->hide();
    ui->deviceComboBox->hide();
    axis_group_->addButton(ui->YRadioButton);
    axis_group_->addButton(ui->ZRadioButton);
    axis_group_->addButton(ui->ARadioButton);
    ui->rollerButton->setIcon(QIcon(isDarkMode() ? ":/resources/images/dark/icon-rotary.png" : ":/resources/images/icon-rotary.png"));
    ui->chuckButton->setIcon(QIcon(isDarkMode() ? ":/resources/images/dark/icon-rotary-2.png" : ":/resources/images/icon-rotary-2.png"));
    ui->rollerButton->setChecked(true);
    if(isDarkMode()) {
        ui->rollerButton->setStyleSheet(selected_dark_);
        ui->chuckButton->setStyleSheet(unselect_dark_);
    } else {
        ui->rollerButton->setStyleSheet(selected_light_);
        ui->chuckButton->setStyleSheet(unselect_light_);
    }
    mm_per_rotation_ = ui->mmPerRotationSpinBox->value();
    roller_diameter_ = ui->rollerDiameterSpinBox->value();
    roller_type_ = true;
    resetUI();
    connect(ui->testBtn, &QAbstractButton::clicked, this, &RotarySetup::testRotary);
    connect(ui->rotaryCheckBox, &QCheckBox::stateChanged, [=](int state){
        is_rotary_mode_ = state;
        if(is_rotary_mode_) {
            ui->mirrorCheckBox->setEnabled(true);
            if(control_enable_) ui->testBtn->setEnabled(true);
        }
        else {
            ui->mirrorCheckBox->setEnabled(false);
            ui->testBtn->setEnabled(false);
        }
        Q_EMIT rotaryModeChanged(is_rotary_mode_);
    });
    connect(ui->rollerButton, &QAbstractButton::clicked, [=](bool checked) {
        ui->rollerButton->setChecked(true);
        ui->chuckButton->setChecked(false);
    });
    connect(ui->chuckButton, &QAbstractButton::clicked, [=](bool checked) {
        ui->chuckButton->setChecked(true);
        ui->rollerButton->setChecked(false);
    });
    connect(ui->rollerButton, &QAbstractButton::toggled, [=](bool checked) {
        if(checked) {
            ui->rollerDiameterSpinBox->setEnabled(true);
            if(isDarkMode()) {
                ui->rollerButton->setStyleSheet(selected_dark_);
                ui->chuckButton->setStyleSheet(unselect_dark_);
            } else {
                ui->rollerButton->setStyleSheet(selected_light_);
                ui->chuckButton->setStyleSheet(unselect_light_);
            }
        }
    });
    connect(ui->chuckButton, &QAbstractButton::toggled, [=](bool checked) {
        if(checked) {
            ui->rollerDiameterSpinBox->setEnabled(false);
            if(isDarkMode()) {
                ui->rollerButton->setStyleSheet(unselect_dark_);
                ui->chuckButton->setStyleSheet(selected_dark_);
            } else {
                ui->rollerButton->setStyleSheet(unselect_light_);
                ui->chuckButton->setStyleSheet(selected_light_);
            }
        }
    });
    connect(ui->CircumSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), [=](double circumference){
        ui->ObjectSpinBox->blockSignals(true);
        ui->ObjectSpinBox->setValue(circumference / M_PI);
        ui->ObjectSpinBox->blockSignals(false);
    });
    connect(ui->ObjectSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), [=](double diameter){
        ui->CircumSpinBox->blockSignals(true);
        ui->CircumSpinBox->setValue(diameter * M_PI);
        ui->CircumSpinBox->blockSignals(false);
    });
    connect(ui->buttonBox, &QDialogButtonBox::accepted, [=](){
        is_mirror_mode_ = ui->mirrorCheckBox->checkState();
        if(ui->YRadioButton->isChecked()) {
            rotary_axis_ = 'Y';
        } else if(ui->ZRadioButton->isChecked()) {
            rotary_axis_ = 'Z';
        } else if(ui->ARadioButton->isChecked()) {
            rotary_axis_ = 'A';
        }
        circumference_ = ui->CircumSpinBox->value();
        if(ui->rollerButton->isChecked()) roller_type_ = true;
        else roller_type_ = false;
        mm_per_rotation_ = ui->mmPerRotationSpinBox->value();
        roller_diameter_ = ui->rollerDiameterSpinBox->value();

        QSettings settings;
        settings.setValue("rotary/circumference", circumference_);

        Q_EMIT mirrorModeChanged(is_mirror_mode_);
        Q_EMIT rotaryAxisChanged(rotary_axis_);
        updateRotaryScale();
        updateCircumference(circumference_);
    });
    connect(ui->buttonBox, &QDialogButtonBox::rejected, [=](){
        resetUI();
    });
}

RotarySetup::~RotarySetup()
{
    delete ui;
}

bool RotarySetup::isRotaryMode()
{
    return is_rotary_mode_;
}

bool RotarySetup::isMirrorMode()
{
    return is_mirror_mode_;
}

char RotarySetup::getRotaryAxis()
{
    return rotary_axis_;
}

void RotarySetup::setRotaryMode(bool is_rotary_mode)
{
    is_rotary_mode_ = is_rotary_mode;
    if(is_rotary_mode_) {
        ui->rotaryCheckBox->setCheckState(Qt::Checked);
        ui->mirrorCheckBox->setEnabled(true);
        if(control_enable_) ui->testBtn->setEnabled(true);
    }
    else {
        ui->rotaryCheckBox->setCheckState(Qt::Unchecked);
        ui->mirrorCheckBox->setEnabled(false);
        ui->testBtn->setEnabled(false);
    }
}

void RotarySetup::setMirrorMode(bool is_mirror_mode)
{
    is_mirror_mode_ = is_mirror_mode;
}

void RotarySetup::setRotaryAxis(char rotary_axis)
{
    switch (rotary_axis) {
        case 'Y':
        case 'Z':
        case 'A':
            rotary_axis_ = rotary_axis;
            break;
        default:
            qInfo() << Q_FUNC_INFO << " get axis: " << rotary_axis << " over range";
            break;
    }
}

double RotarySetup::getCircumference()
{
    return circumference_;
}

double RotarySetup::getRotaryScale()
{
    return rotary_scale_;
}

void RotarySetup::setControlEnable(bool control_enable)
{
    control_enable_ = control_enable;
    if(control_enable_ && is_rotary_mode_) {
        ui->testBtn->setEnabled(true);
        ui->mirrorCheckBox->setEnabled(true);
    }
    else {
        ui->testBtn->setEnabled(false);
        ui->mirrorCheckBox->setEnabled(false);
    }
}

void RotarySetup::setDefaultCircumference(double default_value)
{
    //if the rotary/circumference is not update
    QSettings settings;
    QVariant circumference_data = settings.value("rotary/circumference", 0);
    if(circumference_data.toDouble() > 0) {
        ui->CircumSpinBox->setValue(circumference_data.toDouble());
        circumference_ = circumference_data.toDouble();
    } else {
        ui->CircumSpinBox->setValue(default_value);
        settings.setValue("rotary/circumference", default_value);
        circumference_ = default_value;
    }
    resetUI();
}

/**
 * @brief Go through a rectangular path with its height aligned with rotary axis
 * 
 */
void RotarySetup::testRotary()
{
  QRectF bbox;
  bbox.setWidth(20); // fixed: 20 mm
  bbox.setHeight(ui->mmPerRotationSpinBox->value());
  Q_EMIT actionTestRotary(bbox, rotary_axis_, travel_speed_, framing_power_);
}

void RotarySetup::updateRotaryScale()
{
    if(ui->rollerButton->isChecked()) {
        double circumference = ui->rollerDiameterSpinBox->value() * M_PI;
        if(circumference > 0) {
            rotary_scale_ = ui->mmPerRotationSpinBox->value() / circumference;
        }
        else {
            rotary_scale_ = 0;
        }
    }
    else {
        if(circumference_ > 0) {
            rotary_scale_ = ui->mmPerRotationSpinBox->value() / circumference_;
        }
        else {
            rotary_scale_ = 0;
        }
    }
}

void RotarySetup::resetUI()
{
    if(is_rotary_mode_) {
        ui->rotaryCheckBox->setCheckState(Qt::Checked);
        if(control_enable_) ui->testBtn->setEnabled(true);
        ui->mirrorCheckBox->setEnabled(true);
    }
    else {
        ui->rotaryCheckBox->setCheckState(Qt::Unchecked);
        ui->testBtn->setEnabled(false);
        ui->mirrorCheckBox->setEnabled(false);
    }
    if(is_mirror_mode_) {
        ui->mirrorCheckBox->setCheckState(Qt::Checked);
    }
    else {
        ui->mirrorCheckBox->setCheckState(Qt::Unchecked);
    }
    switch (rotary_axis_) {
        case 'Y':
            ui->YRadioButton->setChecked(true);
            break;
        case 'Z':
            ui->ZRadioButton->setChecked(true);
            break;
        case 'A':
            ui->ARadioButton->setChecked(true);
            break;
    }
    ui->ObjectSpinBox->setValue(circumference_ / M_PI);
    ui->CircumSpinBox->setValue(circumference_);
    if(roller_type_) {
        ui->rollerButton->setChecked(true);
        ui->chuckButton->setChecked(false);
    } else {
        ui->rollerButton->setChecked(false);
        ui->chuckButton->setChecked(true);
    }
    ui->mmPerRotationSpinBox->setValue(mm_per_rotation_);
    ui->rollerDiameterSpinBox->setValue(roller_diameter_);
}
