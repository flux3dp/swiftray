#include <QDebug>
#include "layer_params_panel.h"
#include "ui_layer_params_panel.h"
#include <widgets/spinbox_helper.h>

LayerParamsPanel::LayerParamsPanel(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::LayerParamsPanel) {
    ui->setupUi(this);
    ((SpinBoxHelper<QSpinBox> *)ui->spinBoxPower)->lineEdit()->setStyleSheet("padding: 0 8px;");
    ((SpinBoxHelper<QSpinBox> *)ui->spinBoxRepeat)->lineEdit()->setStyleSheet("padding: 0 8px;");
    ((SpinBoxHelper<QSpinBox> *)ui->spinBoxSpeed)->lineEdit()->setStyleSheet("padding: 0 8px;");
}

void LayerParamsPanel::updateLayer(LayerPtr layer) {
    qInfo() << "Update layer" << layer->name() << layer->speed();
    layer_ = layer;
    ui->spinBoxPower->setValue(layer->strength());
    ui->spinBoxSpeed->setValue(layer->speed());
    ui->spinBoxRepeat->setValue(layer->repeat());
}

LayerParamsPanel::~LayerParamsPanel() {
    delete ui;
}
