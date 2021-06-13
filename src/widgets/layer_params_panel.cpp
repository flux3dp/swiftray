#include <QDebug>
#include "layer_params_panel.h"
#include "ui_layer_params_panel.h"
#include <widgets/spinbox_helper.h>

LayerParamsPanel::LayerParamsPanel(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::LayerParamsPanel) {
    ui->setupUi(this);
    loadStyles();
    registerEvents();
}

LayerParamsPanel::~LayerParamsPanel() {
    delete ui;
}

void LayerParamsPanel::loadStyles() {
    ((SpinBoxHelper<QSpinBox> *)ui->spinBoxPower)->lineEdit()->setStyleSheet("padding: 0 8px;");
    ((SpinBoxHelper<QSpinBox> *)ui->spinBoxRepeat)->lineEdit()->setStyleSheet("padding: 0 8px;");
    ((SpinBoxHelper<QSpinBox> *)ui->spinBoxSpeed)->lineEdit()->setStyleSheet("padding: 0 8px;");
}

void LayerParamsPanel::registerEvents() {
    connect(ui->spinBoxPower, QOverload<int>::of(&QSpinBox::valueChanged), [=] (int strength) {
        if (layer_!= nullptr) layer_->setStrength(strength);
    });
    connect(ui->spinBoxSpeed, QOverload<int>::of(&QSpinBox::valueChanged), [=] (int speed) {
        if (layer_!= nullptr) layer_->setSpeed(speed);
    });
    connect(ui->spinBoxRepeat, QOverload<int>::of(&QSpinBox::valueChanged), [=] (int repeat) {
        if (layer_!= nullptr) layer_->setRepeat(repeat);
    });
}

void LayerParamsPanel::updateLayer(LayerPtr layer) {
    layer_ = layer;
    ui->spinBoxPower->setValue(layer->strength());
    ui->spinBoxSpeed->setValue(layer->speed());
    ui->spinBoxRepeat->setValue(layer->repeat());
}