#include "rotary_setup.h"
#include "ui_rotary_setup.h"

#include <QDebug>

RotarySetup::RotarySetup(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RotarySetup)
{
    ui->setupUi(this);
    ui->label->hide();
    ui->deviceComboBox->hide();
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
    switch (rotary_axis_.at(0).unicode()) {
        case 'X':
            ui->XRadioButton->setChecked(true);
            break;
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
    connect(ui->testBtn, &QAbstractButton::clicked, this, &RotarySetup::testRotaryAxis);
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
    connect(ui->XRadioButton, &QAbstractButton::clicked, [=](bool checked){
        rotary_axis_ = "X";
        Q_EMIT rotaryAxisChanged(rotary_axis_);
    });
    connect(ui->YRadioButton, &QAbstractButton::clicked, [=](bool checked){
        rotary_axis_ = "Y";
        Q_EMIT rotaryAxisChanged(rotary_axis_);
    });
    connect(ui->ZRadioButton, &QAbstractButton::clicked, [=](bool checked){
        rotary_axis_ = "Z";
        Q_EMIT rotaryAxisChanged(rotary_axis_);
    });
    connect(ui->ARadioButton, &QAbstractButton::clicked, [=](bool checked){
        rotary_axis_ = "A";
        Q_EMIT rotaryAxisChanged(rotary_axis_);
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

QString RotarySetup::getRotaryAxis()
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

void RotarySetup::setRotaryAxis(QString rotary_axis)
{
    switch (rotary_axis.at(0).unicode()) {
        case 'X':
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

void RotarySetup::testRotaryAxis()
{
}