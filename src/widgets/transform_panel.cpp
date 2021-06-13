#include "transform_panel.h"
#include "ui_transform_panel.h"
#include <widgets/spinbox_helper.h>

TransformPanel::TransformPanel(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::TransformPanel) {
    ui->setupUi(this);
    loadStyles();
    registerEvents();
}

TransformPanel::~TransformPanel() {
    delete ui;
}

void TransformPanel::loadStyles() {
    ((SpinBoxHelper<QDoubleSpinBox> *)ui->spinBoxX)->lineEdit()->setStyleSheet("padding: 0 3px;");
    ((SpinBoxHelper<QDoubleSpinBox> *)ui->spinBoxY)->lineEdit()->setStyleSheet("padding: 0 3px;");
    ((SpinBoxHelper<QDoubleSpinBox> *)ui->spinBoxR)->lineEdit()->setStyleSheet("padding: 0 3px;");
    ((SpinBoxHelper<QDoubleSpinBox> *)ui->spinBoxW)->lineEdit()->setStyleSheet("padding: 0 3px;");
    ((SpinBoxHelper<QDoubleSpinBox> *)ui->spinBoxH)->lineEdit()->setStyleSheet("padding: 0 3px;");
}

void TransformPanel::registerEvents() {
    auto spin_event = QOverload<double>::of(&QDoubleSpinBox::valueChanged);
    connect(ui->spinBoxX, spin_event, [=](double value) {
        if (x_ != value) {
            x_ = value;
            updateControl();
        }
    });
    connect(ui->spinBoxY, spin_event, [=](double value) {
        if (y_ != value) {
            y_ = value;
            updateControl();
        }
    });
    connect(ui->spinBoxR, spin_event, [=](double value) {
        if (r_ != value) {
            r_ = value;
            updateControl();
        }
    });
    connect(ui->spinBoxW, spin_event, [=](double value) {
        if (value == 0) return;
        if (w_ != value) {
            w_ = value;
            updateControl();
        }
    });
    connect(ui->spinBoxH, spin_event, [=](double value) {
        if (value == 0) return;
        if (h_ != value) {
            h_ = value;
            updateControl();
        }
    });
}

bool TransformPanel::isScaleLock() const {
    return scale_lock_;
}

void TransformPanel::setScaleLock(bool scaleLock) {
    scale_lock_ = scaleLock;
}

void TransformPanel::setTransformControl(Controls::Transform *ctrl) {
    ctrl_ = ctrl;

    connect(ctrl, &Controls::Transform::transformChanged, [=](void) {
        x_ = ctrl_->x();
        y_ = ctrl_->y();
        w_ = ctrl_->width();
        h_ = ctrl_->height();
        r_ = ctrl_->rotation();
        ui->spinBoxX->setValue(x_);
        ui->spinBoxY->setValue(y_);
        ui->spinBoxR->setValue(r_);
        ui->spinBoxW->setValue(w_);
        ui->spinBoxH->setValue(h_);
    });
}

void TransformPanel::updateControl() {
    qInfo() << "PX" << x_ << y_ << r_ << w_ << h_;
    ctrl_->updateTransformFromUI(x_, y_, r_, w_, h_);
}

