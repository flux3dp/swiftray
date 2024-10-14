#include "image-panel.h"
#include "ui_image-panel.h"

#include <QDebug>

ImagePanel::ImagePanel(QWidget *parent, bool is_dark_mode) :
    QFrame(parent),
    ui(new Ui::ImagePanel)
{
    ui->setupUi(this);
    initializeContainer();
    setLayout(is_dark_mode);
}

ImagePanel::~ImagePanel()
{
    delete ui;
}

void ImagePanel::loadStyles() {
  // ui->frameButtons->setStyleSheet("\
  //     QpushButton {   \
  //         border: none \
  //     } \
  //     QpushButton:checked{ \
  //         border: none \
  //     } \
  // ");
}

void ImagePanel::registerEvents() {
  connect(ui->gradientCheckBox, QOverload<int>::of(&QCheckBox::stateChanged), [=](int state) {
    Q_EMIT editImageGradient(state);
  });

  connect(ui->brightnessThresholdSlider, QOverload<int>::of(&QSlider::valueChanged), [=](int value) {
    Q_EMIT editImageThreshold(value);
  });

  connect(ui->pushButtonCrop, &QAbstractButton::clicked, [=]() {
    Q_EMIT actionCropImage();
  });
  connect(ui->pushButtonInvert, &QAbstractButton::clicked, [=]() {
    Q_EMIT actionInvertImage();
  });
  connect(ui->pushButtonSharpen, &QAbstractButton::clicked, [=]() {
    Q_EMIT actionSharpenImage();
  });
  connect(ui->pushButtonTrace, &QAbstractButton::clicked, [=]() {
    Q_EMIT actionGenImageTrace();
  });
}

void ImagePanel::setLayout(bool is_dark_mode) {
  ui->pushButtonCrop->setIcon(QIcon(is_dark_mode ? ":/resources/images/dark/icon-crop.png" : ":/resources/images/icon-crop.png"));
  ui->pushButtonInvert->setIcon(QIcon(is_dark_mode ? ":/resources/images/dark/icon-invert.png" : ":/resources/images/icon-invert.png"));
  ui->pushButtonSharpen->setIcon(QIcon(is_dark_mode ? ":/resources/images/dark/icon-sharpen.png" : ":/resources/images/icon-sharpen.png"));
  ui->pushButtonTrace->setIcon(QIcon(is_dark_mode ? ":/resources/images/dark/icon-trace.png" : ":/resources/images/icon-trace.png"));
}

void ImagePanel::setImageGradient(bool state) {
  ui->gradientCheckBox->blockSignals(true);
  if(state) {
    ui->gradientCheckBox->setCheckState(Qt::Checked);
  } else {
    ui->gradientCheckBox->setCheckState(Qt::Unchecked);
  }
  ui->brightnessThresholdGroup->setEnabled(!state);
  ui->brightnessThresholdSlider->setEnabled(!state);
  ui->gradientCheckBox->blockSignals(false);
}

void ImagePanel::setImageThreshold(int value) {
  ui->brightnessThresholdSlider->blockSignals(true);
  ui->brightnessThresholdSlider->setSliderPosition(value);
  ui->brightnessThresholdSlider->blockSignals(false);
}

void ImagePanel::changeImageEnable(bool enable) {
  ui->scrollAreaWidgetContents->setEnabled(enable);
}

void ImagePanel::hideEvent(QHideEvent *event) {
  Q_EMIT panelShow(false);
}

void ImagePanel::showEvent(QShowEvent *event) {
  Q_EMIT panelShow(true);
}
