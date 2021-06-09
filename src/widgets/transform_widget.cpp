#include "transform_widget.h"
#include "ui_transform_widget.h"
#include <widgets/spinbox_helper.h>

TransformWidget::TransformWidget(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::TransformWidget)
{
    ui->setupUi(this);

    ((SpinBoxHelper<QDoubleSpinBox> *)ui->doubleSpinBox)->lineEdit()->setStyleSheet("padding: 0 3px;");
    ((SpinBoxHelper<QDoubleSpinBox> *)ui->doubleSpinBox_2)->lineEdit()->setStyleSheet("padding: 0 3px;");
    ((SpinBoxHelper<QDoubleSpinBox> *)ui->doubleSpinBox_3)->lineEdit()->setStyleSheet("padding: 0 3px;");
    ((SpinBoxHelper<QDoubleSpinBox> *)ui->doubleSpinBox_4)->lineEdit()->setStyleSheet("padding: 0 3px;");
    ((SpinBoxHelper<QDoubleSpinBox> *)ui->doubleSpinBox_5)->lineEdit()->setStyleSheet("padding: 0 3px;");
}

TransformWidget::~TransformWidget()
{
    delete ui;
}
