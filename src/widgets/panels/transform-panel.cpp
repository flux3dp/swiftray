#include "transform-panel.h"
#include "ui_transform-panel.h"
#include <widgets/components/spinbox-helper.h>
#include <canvas/canvas.h>

TransformPanel::TransformPanel(QWidget *parent, Canvas *canvas) :
     QFrame(parent),
     canvas_(canvas),
     ui(new Ui::TransformPanel) {
  assert(parent != nullptr && canvas != nullptr);
  ui->setupUi(this);
  loadStyles();
  registerEvents();
}

TransformPanel::~TransformPanel() {
  delete ui;
}

void TransformPanel::loadStyles() {
  ((SpinBoxHelper<QDoubleSpinBox> *) ui->xSpinBox)->lineEdit()->setStyleSheet("padding: 0 3px;");
  ((SpinBoxHelper<QDoubleSpinBox> *) ui->ySpinBox)->lineEdit()->setStyleSheet("padding: 0 3px;");
  ((SpinBoxHelper<QDoubleSpinBox> *) ui->rotationSpinBox)->lineEdit()->setStyleSheet("padding: 0 3px;");
  ((SpinBoxHelper<QDoubleSpinBox> *) ui->widthSpinBox)->lineEdit()->setStyleSheet("padding: 0 3px;");
  ((SpinBoxHelper<QDoubleSpinBox> *) ui->heightSpinBox)->lineEdit()->setStyleSheet("padding: 0 3px;");
}

void TransformPanel::registerEvents() {
  auto spin_event = QOverload<double>::of(&QDoubleSpinBox::valueChanged);
  connect(ui->xSpinBox, spin_event, [=](double value) {
    if (x_ != value) {
      x_ = value;
      updateControl();
    }
  });

  connect(ui->ySpinBox, spin_event, [=](double value) {
    if (y_ != value) {
      y_ = value;
      updateControl();
    }
  });

  connect(ui->rotationSpinBox, spin_event, [=](double value) {
    if (r_ != value) {
      r_ = value;
      updateControl();
    }
  });

  connect(ui->widthSpinBox, spin_event, [=](double value) {
    if (value == 0) return;
    if (w_ != value) {
      w_ = value;
      updateControl();
    }
  });

  connect(ui->heightSpinBox, spin_event, [=](double value) {
    if (value == 0) return;
    if (h_ != value) {
      h_ = value;
      updateControl();
    }
  });

  connect(canvas_, &Canvas::transformChanged, [=](qreal x, qreal y, qreal r, qreal w, qreal h) {
    x_ = x;
    y_ = y;
    r_ = r;
    w_ = w;
    h_ = h;
    ui->xSpinBox->setValue(x_);
    ui->ySpinBox->setValue(y_);
    ui->rotationSpinBox->setValue(r_);
    ui->widthSpinBox->setValue(w_);
    ui->heightSpinBox->setValue(h_);
  });
}

bool TransformPanel::isScaleLock() const {
  return scale_locked_;
}

void TransformPanel::setScaleLock(bool scaleLock) {
  scale_locked_ = scaleLock;
}

void TransformPanel::updateControl() {
  canvas_->transformControl().updateTransform(x_, y_, r_, w_, h_);
}

