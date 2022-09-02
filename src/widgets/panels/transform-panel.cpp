#include "transform-panel.h"
#include "ui_transform-panel.h"
#include <windows/mainwindow.h>
#include <windows/osxwindow.h>

TransformPanel::TransformPanel(QWidget *parent, MainWindow *main_window) :
     QFrame(parent),
     main_window_(main_window),
     ui(new Ui::TransformPanel),
     scale_locked_(false),
     BaseContainer() {
  assert(parent != nullptr && main_window != nullptr);
  ui->setupUi(this);
  initializeContainer();
  setLayout();
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
      if (scale_locked_) {
        h_ = w_ == 0 ? 0 : h_ * value / w_;
      }
      w_ = value;
      updateControl();
    }
  });

  connect(ui->heightSpinBox, spin_event, [=](double value) {
    if (value == 0) return;
    if (h_ != value) {
      if (scale_locked_) {
        w_ = h_ == 0 ? 0 : w_ * value / h_;
      }
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
    ui->xSpinBox->blockSignals(true);
    ui->ySpinBox->blockSignals(true);
    ui->rotationSpinBox->blockSignals(true);
    ui->widthSpinBox->blockSignals(true);
    ui->heightSpinBox->blockSignals(true);
    ui->xSpinBox->setValue(x_);
    ui->ySpinBox->setValue(y_);
    ui->rotationSpinBox->setValue(r_);
    ui->widthSpinBox->setValue(w_);
    ui->heightSpinBox->setValue(h_);
    ui->xSpinBox->blockSignals(false);
    ui->ySpinBox->blockSignals(false);
    ui->rotationSpinBox->blockSignals(false);
    ui->widthSpinBox->blockSignals(false);
    ui->heightSpinBox->blockSignals(false);
  });

  connect(main_window_, &MainWindow::toolbarTransformChanged, [=](double x, double y, double r, double w, double h) {
    x_ = x;
    y_ = y;
    r_ = r;
    w_ = w;
    h_ = h;
    ui->xSpinBox->setValue(x);
    ui->ySpinBox->setValue(y);
    ui->rotationSpinBox->setValue(r);
    ui->widthSpinBox->setValue(w);
    ui->heightSpinBox->setValue(h);
  });

  connect(ui->lockBtn, &QToolButton::toggled, [=](bool checked) {
    ui->lockBtn->setChecked(checked);
    setScaleLock(checked);
  });
}

bool TransformPanel::isScaleLock() const {
  return scale_locked_;
}

void TransformPanel::setScaleLock(bool scaleLock) {
  scale_locked_ = scaleLock;
  main_window_->canvas()->transformControl().setScaleLock(scaleLock);
  ui->lockBtn->setChecked(scaleLock);

  if (scaleLock) {
    ui->lockBtn->setIcon(QIcon(isDarkMode() ? ":/resources/images/dark/icon-lock.png" : ":/resources/images/icon-lock.png"));
  } else {
    ui->lockBtn->setIcon(QIcon(isDarkMode() ? ":/resources/images/dark/icon-unlock.png" : ":/resources/images/icon-unlock.png"));
  }

  emit scaleLockToggled(scale_locked_);
}

void TransformPanel::updateControl() {
  main_window_->canvas()->transformControl().updateTransform(x_ * 10, y_ * 10, r_, w_ * 10, h_ * 10);
  emit transformPanelUpdated(x_, y_, r_, w_, h_);
}

void TransformPanel::setLayout() {
  ui->lockBtn->setIcon(QIcon(isDarkMode() ? ":/resources/images/dark/icon-unlock.png" : ":/resources/images/icon-unlock.png"));
}
