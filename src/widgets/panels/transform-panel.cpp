#include "transform-panel.h"
#include "ui_transform-panel.h"
#include <constants.h>

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
  connect(ui->NWRadioButton, &QAbstractButton::clicked, [=](bool checked) {
    Q_EMIT editShapeReference(NW);
  });
  connect(ui->NRadioButton, &QAbstractButton::clicked, [=](bool checked) {
    Q_EMIT editShapeReference(N);
  });
  connect(ui->NERadioButton, &QAbstractButton::clicked, [=](bool checked) {
    Q_EMIT editShapeReference(NE);
  });
  connect(ui->WRadioButton, &QAbstractButton::clicked, [=](bool checked) {
    Q_EMIT editShapeReference(W);
  });
  connect(ui->CRadioButton, &QAbstractButton::clicked, [=](bool checked) {
    Q_EMIT editShapeReference(CENTER);
  });
  connect(ui->ERadioButton, &QAbstractButton::clicked, [=](bool checked) {
    Q_EMIT editShapeReference(E);
  });
  connect(ui->SWRadioButton, &QAbstractButton::clicked, [=](bool checked) {
    Q_EMIT editShapeReference(SW);
  });
  connect(ui->SRadioButton, &QAbstractButton::clicked, [=](bool checked) {
    Q_EMIT editShapeReference(S);
  });
  connect(ui->SERadioButton, &QAbstractButton::clicked, [=](bool checked) {
    Q_EMIT editShapeReference(SE);
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

void TransformPanel::changeTransformEnable(bool enable) {
  ui->scrollAreaWidgetContents->setEnabled(enable);
}

void TransformPanel::setShapeReference(int shape_reference) {
  switch(shape_reference) {
    case NW:
      ui->NWRadioButton->blockSignals(true);
      ui->NWRadioButton->setChecked(true);
      ui->NWRadioButton->blockSignals(false);
      break;
    case N:
      ui->NRadioButton->blockSignals(true);
      ui->NRadioButton->setChecked(true);
      ui->NRadioButton->blockSignals(false);
      break;
    case NE:
      ui->NERadioButton->blockSignals(true);
      ui->NERadioButton->setChecked(true);
      ui->NERadioButton->blockSignals(false);
      break;
    case E:
      ui->ERadioButton->blockSignals(true);
      ui->ERadioButton->setChecked(true);
      ui->ERadioButton->blockSignals(false);
      break;
    case SE:
      ui->SERadioButton->blockSignals(true);
      ui->SERadioButton->setChecked(true);
      ui->SERadioButton->blockSignals(false);
      break;
    case S:
      ui->SRadioButton->blockSignals(true);
      ui->SRadioButton->setChecked(true);
      ui->SRadioButton->blockSignals(false);
      break;
    case SW:
      ui->SWRadioButton->blockSignals(true);
      ui->SWRadioButton->setChecked(true);
      ui->SWRadioButton->blockSignals(false);
      break;
    case W:
      ui->WRadioButton->blockSignals(true);
      ui->WRadioButton->setChecked(true);
      ui->WRadioButton->blockSignals(false);
      break;
    case CENTER:
      ui->CRadioButton->blockSignals(true);
      ui->CRadioButton->setChecked(true);
      ui->CRadioButton->blockSignals(false);
      break;
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
