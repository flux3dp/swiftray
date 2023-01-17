#include "transform-panel.h"
#include "ui_transform-panel.h"

TransformPanel::TransformPanel(QWidget *parent, bool is_dark_mode) :
     QFrame(parent),
     ui(new Ui::TransformPanel),
     BaseContainer() {
  assert(parent != nullptr);
  ui->setupUi(this);
  initializeContainer();
  setLayout(is_dark_mode);
}

TransformPanel::~TransformPanel() {
  delete ui;
}

void TransformPanel::loadStyles() {
}

void TransformPanel::registerEvents() {
  auto spin_event = QOverload<double>::of(&QDoubleSpinBox::valueChanged);
  connect(ui->xSpinBox, spin_event, [=](double value) {
    Q_EMIT editShapeTransformX(value);
  });

  connect(ui->ySpinBox, spin_event, [=](double value) {
    Q_EMIT editShapeTransformY(value);
  });

  connect(ui->rotationSpinBox, spin_event, [=](double value) {
    Q_EMIT editShapeTransformR(value);
  });

  connect(ui->widthSpinBox, spin_event, [=](double value) {
    Q_EMIT editShapeTransformW(value);
  });

  connect(ui->heightSpinBox, spin_event, [=](double value) {
    Q_EMIT editShapeTransformH(value);
  });

  connect(ui->lockBtn, &QToolButton::toggled, [=](bool checked) {
    setScaleLock(checked);
    Q_EMIT scaleLockToggled(checked);
  });
}

void TransformPanel::setTransformX(double x) {
  ui->xSpinBox->blockSignals(true);
  ui->xSpinBox->setValue(x);
  ui->xSpinBox->blockSignals(false);
}

void TransformPanel::setTransformY(double y) {
  ui->ySpinBox->blockSignals(true);
  ui->ySpinBox->setValue(y);
  ui->ySpinBox->blockSignals(false);
}

void TransformPanel::setTransformR(double r) {
  ui->rotationSpinBox->blockSignals(true);
  ui->rotationSpinBox->setValue(r);
  ui->rotationSpinBox->blockSignals(false);
}

void TransformPanel::setTransformW(double w) {
  ui->widthSpinBox->blockSignals(true);
  ui->widthSpinBox->setValue(w);
  ui->widthSpinBox->blockSignals(false);
}

void TransformPanel::setTransformH(double h) {
  ui->heightSpinBox->blockSignals(true);
  ui->heightSpinBox->setValue(h);
  ui->heightSpinBox->blockSignals(false);
}

void TransformPanel::setScaleLock(bool scaleLock) {
  ui->lockBtn->blockSignals(true);
  ui->lockBtn->setChecked(scaleLock);
  ui->lockBtn->blockSignals(false);

  if (scaleLock) {
    ui->lockBtn->setIcon(QIcon(is_dark_mode_ ? ":/resources/images/dark/icon-lock.png" : ":/resources/images/icon-lock.png"));
  } else {
    ui->lockBtn->setIcon(QIcon(is_dark_mode_ ? ":/resources/images/dark/icon-unlock.png" : ":/resources/images/icon-unlock.png"));
  }
}

void TransformPanel::setLayout(bool is_dark_mode) {
  is_dark_mode_ = is_dark_mode;
  ui->lockBtn->setIcon(QIcon(is_dark_mode ? ":/resources/images/dark/icon-unlock.png" : ":/resources/images/icon-unlock.png"));
}

void TransformPanel::hideEvent(QHideEvent *event) {
  Q_EMIT panelShow(false);
}

void TransformPanel::showEvent(QShowEvent *event) {
  Q_EMIT panelShow(true);
}
