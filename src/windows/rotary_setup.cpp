#include "rotary_setup.h"
#include "ui_rotary_setup.h"
#include <math.h>

#include <QDebug>

RotarySetup::RotarySetup(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RotarySetup)
{
    ui->setupUi(this);
    axis_group_ = new QButtonGroup(this);
    rotation_group_ = new QButtonGroup(this);
    ui->label->hide();
    ui->deviceComboBox->hide();
    ui->mirrorCheckBox->hide();
    ui->label_2->hide();
    ui->typeList->hide();
    axis_group_->addButton(ui->YRadioButton);
    axis_group_->addButton(ui->ZRadioButton);
    axis_group_->addButton(ui->ARadioButton);
    rotation_group_->addButton(ui->rollerRadioButton);
    rotation_group_->addButton(ui->objectRadioButton);
    ui->rollerRadioButton->setChecked(true);
    if(is_rotary_mode_) {
        ui->rotaryCheckBox->setCheckState(Qt::Checked);
        ui->testBtn->setEnabled(true);
    }
    else {
        ui->rotaryCheckBox->setCheckState(Qt::Unchecked);
        ui->testBtn->setEnabled(false);
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
    connect(ui->testBtn, &QAbstractButton::clicked, this, &RotarySetup::testRotary);
    connect(ui->rotaryCheckBox, &QCheckBox::stateChanged, [=](int state){
        is_rotary_mode_ = state;
        if(is_rotary_mode_) {
            ui->testBtn->setEnabled(true);
        }
        else {
            ui->testBtn->setEnabled(false);
        }
        Q_EMIT rotaryModeChanged(is_rotary_mode_);
    });
    connect(ui->mirrorCheckBox, &QCheckBox::stateChanged, [=](int state){
        is_mirror_mode_ = state;
        Q_EMIT mirrorModeChanged(is_mirror_mode_);
    });
    connect(ui->YRadioButton, &QAbstractButton::clicked, [=](bool checked){
        rotary_axis_ = 'Y';
        Q_EMIT rotaryAxisChanged(rotary_axis_);
    });
    connect(ui->ZRadioButton, &QAbstractButton::clicked, [=](bool checked){
        rotary_axis_ = 'Z';
        Q_EMIT rotaryAxisChanged(rotary_axis_);
    });
    connect(ui->ARadioButton, &QAbstractButton::clicked, [=](bool checked){
        rotary_axis_ = 'A';
        Q_EMIT rotaryAxisChanged(rotary_axis_);
    });
    connect(ui->rollerRadioButton, &QAbstractButton::clicked, [=](bool checked){
        if(checked) {
            ui->rollerDiameterSpinBox->setEnabled(true);
        }
        else {
            ui->rollerDiameterSpinBox->setEnabled(false);
        }
        updateRotaryScale();
    });
    connect(ui->objectRadioButton, &QAbstractButton::clicked, [=](bool checked){
        if(checked) {
            ui->rollerDiameterSpinBox->setEnabled(false);
        }
        else {
            ui->rollerDiameterSpinBox->setEnabled(true);
        }
        updateRotaryScale();
    });
    connect(ui->CircumSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), [=](double circumference){
        circumference_ = circumference;
        double object_diameter = circumference_ / M_PI;
        ui->ObjectSpinBox->blockSignals(true);
        ui->ObjectSpinBox->setValue(object_diameter);
        ui->ObjectSpinBox->blockSignals(false);
        updateRotaryScale();
        updateCircumference(circumference_);
    });
    connect(ui->ObjectSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), [=](double diameter){
        circumference_ = diameter * M_PI;
        ui->CircumSpinBox->blockSignals(true);
        ui->CircumSpinBox->setValue(circumference_);
        ui->CircumSpinBox->blockSignals(false);
        updateRotaryScale();
        updateCircumference(circumference_);
    });
    connect(ui->mmPerRotationSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), [=](double move){
        updateRotaryScale();
    });
    connect(ui->rollerDiameterSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), [=](double diameter) {
        updateRotaryScale();
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
        ui->testBtn->setEnabled(true);
    }
    else {
        ui->rotaryCheckBox->setCheckState(Qt::Unchecked);
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

/**
 * @brief Go through a rectangular path with its height aligned with rotary axis
 * 
 */
void RotarySetup::testRotary()
{
  QRectF bbox;
  bbox.setWidth(20); // fixed: 20 mm
  bbox.setHeight(ui->mmPerRotationSpinBox->value());
  emit actionTestRotary(bbox, rotary_axis_, travel_speed_);
}

void RotarySetup::updateRotaryScale()
{
    if(ui->rollerRadioButton->isChecked()) {
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