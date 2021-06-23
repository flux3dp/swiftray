#include "transform-panel.h"
#include "ui_transform-panel.h"
#include <widgets/spinbox-helper.h>
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
  ((SpinBoxHelper<QDoubleSpinBox> *) ui->spinBoxX)->lineEdit()->setStyleSheet("padding: 0 3px;");
  ((SpinBoxHelper<QDoubleSpinBox> *) ui->spinBoxY)->lineEdit()->setStyleSheet("padding: 0 3px;");
  ((SpinBoxHelper<QDoubleSpinBox> *) ui->spinBoxR)->lineEdit()->setStyleSheet("padding: 0 3px;");
  ((SpinBoxHelper<QDoubleSpinBox> *) ui->spinBoxW)->lineEdit()->setStyleSheet("padding: 0 3px;");
  ((SpinBoxHelper<QDoubleSpinBox> *) ui->spinBoxH)->lineEdit()->setStyleSheet("padding: 0 3px;");
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

  connect(canvas_, &Canvas::transformChanged, [=](qreal x, qreal y, qreal r, qreal w, qreal h) {
    x_ = x;
    y_ = y;
    r_ = r;
    w_ = w;
    h_ = h;
    ui->spinBoxX->setValue(x_);
    ui->spinBoxY->setValue(y_);
    ui->spinBoxR->setValue(r_);
    ui->spinBoxW->setValue(w_);
    ui->spinBoxH->setValue(h_);
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

