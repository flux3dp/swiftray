#include "transform_panel.h"
#include "ui_transform_panel.h"
#include <widgets/spinbox_helper.h>

TransformPanel::TransformPanel(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::TransformPanel) {
    ui->setupUi(this);
    ((SpinBoxHelper<QDoubleSpinBox> *)ui->doubleSpinBox)->lineEdit()->setStyleSheet("padding: 0 3px;");
    ((SpinBoxHelper<QDoubleSpinBox> *)ui->doubleSpinBox_2)->lineEdit()->setStyleSheet("padding: 0 3px;");
    ((SpinBoxHelper<QDoubleSpinBox> *)ui->doubleSpinBox_3)->lineEdit()->setStyleSheet("padding: 0 3px;");
    ((SpinBoxHelper<QDoubleSpinBox> *)ui->doubleSpinBox_4)->lineEdit()->setStyleSheet("padding: 0 3px;");
    ((SpinBoxHelper<QDoubleSpinBox> *)ui->doubleSpinBox_5)->lineEdit()->setStyleSheet("padding: 0 3px;");
}

TransformPanel::~TransformPanel() {
    delete ui;
}
