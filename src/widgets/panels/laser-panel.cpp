#include "laser-panel.h"
#include "ui_laser-panel.h"
#include <constants.h>
#include <QDebug>

LaserPanel::LaserPanel(QWidget *parent, bool is_dark_mode) :
  QFrame(parent),
  ui(new Ui::LaserPanel)
{
  ui->setupUi(this);
  initializeContainer();
  qRegisterMetaType<int>();
  ui->startFromComboBox->addItem(tr("Absolute Coords"), StartFrom::AbsoluteCoords);
  ui->startFromComboBox->addItem(tr("User Origin"), StartFrom::UserOrigin);
  ui->startFromComboBox->addItem(tr("Current Position"), StartFrom::CurrentPosition);
  setLayout(is_dark_mode);
}

void LaserPanel::loadStyles() {
}

void LaserPanel::registerEvents() {
  connect(ui->previewBtn, &QAbstractButton::clicked, [=]() {
    Q_EMIT actionPreview();
  });
  connect(ui->frameBtn, &QAbstractButton::clicked, [=]() {
    Q_EMIT actionFrame();
  });
  connect(ui->startBtn, &QAbstractButton::clicked, [=]() {
    Q_EMIT actionStart();
  });
  connect(ui->homeBtn, &QAbstractButton::clicked, [=]() {
    Q_EMIT actionHome();
  });
  connect(ui->moveToOriginBtn, &QAbstractButton::clicked, [=]() {
    Q_EMIT actionMoveToOrigin();
  });
  connect(ui->NWRadioButton, &QAbstractButton::clicked, [=](bool checked) {
    Q_EMIT selectJobOrigin(NW);
  });
  connect(ui->NRadioButton, &QAbstractButton::clicked, [=](bool checked) {
    Q_EMIT selectJobOrigin(N);
  });
  connect(ui->NERadioButton, &QAbstractButton::clicked, [=](bool checked) {
    Q_EMIT selectJobOrigin(NE);
  });
  connect(ui->WRadioButton, &QAbstractButton::clicked, [=](bool checked) {
    Q_EMIT selectJobOrigin(W);
  });
  connect(ui->CRadioButton, &QAbstractButton::clicked, [=](bool checked) {
    Q_EMIT selectJobOrigin(CENTER);
  });
  connect(ui->ERadioButton, &QAbstractButton::clicked, [=](bool checked) {
    Q_EMIT selectJobOrigin(E);
  });
  connect(ui->SWRadioButton, &QAbstractButton::clicked, [=](bool checked) {
    Q_EMIT selectJobOrigin(SW);
  });
  connect(ui->SRadioButton, &QAbstractButton::clicked, [=](bool checked) {
    Q_EMIT selectJobOrigin(S);
  });
  connect(ui->SERadioButton, &QAbstractButton::clicked, [=](bool checked) {
    Q_EMIT selectJobOrigin(SE);
  });
  connect(ui->startFromComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index) {
    setStartFrom(index);
    Q_EMIT switchStartFrom(index);
  });
  connect(ui->homeCheckBox, &QAbstractButton::clicked, [=](bool checked) {
    Q_EMIT startWithHome(checked);
  });
}

LaserPanel::~LaserPanel() {
  delete ui;
}

void LaserPanel::setJobOrigin(int position) {
  switch(position) {
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

void LaserPanel::setStartFrom(int start_from) {
  ui->startFromComboBox->blockSignals(true);
  ui->startFromComboBox->setCurrentIndex(start_from);
  ui->startFromComboBox->blockSignals(false);
  bool start_with_home = ui->homeCheckBox->checkState() && ui->homeCheckBox->isEnabled();
  if(start_from == AbsoluteCoords) {
    ui->widget->hide();
    ui->widget_2->show();
    start_with_home = ui->homeCheckBox->checkState() && ui->homeCheckBox->isEnabled();
  } else {
    ui->widget->show();
    ui->widget_2->hide();
    start_with_home = false;
  }
  Q_EMIT startWithHome(start_with_home);
}

void LaserPanel::setStartHome(bool find_home) {
  ui->homeCheckBox->blockSignals(true);
  if(find_home) {
    ui->homeCheckBox->setCheckState(Qt::Checked);
  } else {
    ui->homeCheckBox->setCheckState(Qt::Unchecked);
  }
  ui->homeCheckBox->blockSignals(false);
}

void LaserPanel::setStartHomeEnable(bool enable) {
  ui->homeCheckBox->setEnabled(enable);
}

bool LaserPanel::getStartHome() {
  return ui->homeCheckBox->checkState();
}

void LaserPanel::setControlEnable(bool enable) {
  ui->frameBtn->setEnabled(enable);
  ui->homeBtn->setEnabled(enable);
  ui->moveToOriginBtn->setEnabled(enable);
}

void LaserPanel::setLayout(bool is_dark_mode) {
  ui->previewBtn->setIcon(QIcon(is_dark_mode ? ":/resources/images/dark/icon-preview.png" : ":/resources/images/icon-preview.png"));
  ui->frameBtn->setIcon(QIcon(is_dark_mode ? ":/resources/images/dark/icon-frame.png" : ":/resources/images/icon-frame.png"));
  ui->startBtn->setIcon(QIcon(is_dark_mode ? ":/resources/images/dark/icon-start.png" : ":/resources/images/icon-start.png"));
}

void LaserPanel::hideEvent(QHideEvent *event) {
  Q_EMIT panelShow(false);
}

void LaserPanel::showEvent(QShowEvent *event) {
  Q_EMIT panelShow(true);
}
