#include "transform-panel.h"
#include "ui_transform-panel.h"
#include <widgets/components/spinbox-helper.h>
#include <windows/mainwindow.h>

TransformPanel::TransformPanel(QWidget *parent, MainWindow *main_window) :
     QFrame(parent),
     main_window_(main_window),
     ui(new Ui::TransformPanel) {
  assert(parent != nullptr && main_window != nullptr);
  ui->setupUi(this);
  loadStyles();
  registerEvents();
}

TransformPanel::~TransformPanel() {
  delete ui;
}

void TransformPanel::loadStyles() {
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

  connect(main_window_->canvas(), &Canvas::transformChanged, [=](qreal x, qreal y, qreal r, qreal w, qreal h) {
    x_ = x / 10;
    y_ = y / 10;
    r_ = r;
    w_ = w / 10;
    h_ = h / 10;
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
  main_window_->canvas()->transformControl().updateTransform(x_ * 10, y_ * 10, r_, w_ * 10, h_ * 10);
}

