#include "image-trace-dialog.h"
#include "ui_image-trace-dialog.h"
#include <QPushButton>

//#include <opencv2/opencv.hpp>
//#include <opencv2/imgproc.hpp>
#include <memory>
#include <QDebug>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QPixmap>
#include <widgets/components/image-trace-graphicsview.h>


ImageTraceDialog::ImageTraceDialog(QWidget *parent) :
        QDialog(parent), ui(new Ui::ImageTraceDialog) {
  ui->setupUi(this);

  registerEvents();

  QGraphicsScene* scene = new QGraphicsScene();
  ui->traceGraphicsView->setScene(scene);
}

ImageTraceDialog::~ImageTraceDialog() {
  delete ui;
}

void ImageTraceDialog::resetParams() {
  ui->cutoffSpinBox->setValue(0);
  ui->cutoffSlider->setValue(0);
  ui->thresholdSlider->setValue(128);
  ui->thresholdSpinBox->setValue(128);
  ui->ignoreSpinBox->setValue(2);
  ui->smoothnessDoubleSpinBox->setValue(1.0);
  ui->optimizeDoubleSpinBox->setValue(0.2);
}

void ImageTraceDialog::onCutoffChanged(int new_cutoff_val) {
  ui->cutoffSpinBox->setValue(new_cutoff_val);
  ui->cutoffSlider->setValue(new_cutoff_val);
  // Cutoff (low threshold) should always <= (high) threshold
  if (new_cutoff_val > ui->thresholdSlider->value()) {
    ui->thresholdSlider->setValue(new_cutoff_val);
    ui->thresholdSpinBox->setValue(new_cutoff_val);
  }
  ui->traceGraphicsView->setThresholds(ui->cutoffSpinBox->value(), ui->thresholdSpinBox->value());
  if (ui->bgImageComboBox->currentIndex() == 1) {
    updateBackgroundDisplay();
  }
  updateImageTrace();
}

void ImageTraceDialog::onThresholdChanged(int new_thres_val) {
  ui->thresholdSlider->setValue(new_thres_val);
  ui->thresholdSpinBox->setValue(new_thres_val);
  // Cutoff (low threshold) should always <= (high) threshold
  if (new_thres_val < ui->cutoffSlider->value()) {
    ui->cutoffSlider->setValue(new_thres_val);
    ui->cutoffSpinBox->setValue(new_thres_val);
  }
  ui->traceGraphicsView->setThresholds(ui->cutoffSpinBox->value(), ui->thresholdSpinBox->value());
  if (ui->bgImageComboBox->currentIndex() == 1) {
    updateBackgroundDisplay();
  }
  updateImageTrace();
}

void ImageTraceDialog::onSelectPartialStateChanged(int check_state) {
  ui->traceGraphicsView->clearPartialSelect();
  if (check_state == Qt::Checked) {
    ui->traceGraphicsView->setDragMode(QGraphicsView::RubberBandDrag);
  } else if (check_state == Qt::Unchecked) {
    ui->traceGraphicsView->setDragMode(QGraphicsView::ScrollHandDrag);
  }
  this->updateImageTrace();
}

void ImageTraceDialog::registerEvents() {
  connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, [=](){
    // TODO: Convert trace paths to shape on Canvas

    this->close();
  });
  connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &QDialog::close);

  connect(ui->cutoffSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &ImageTraceDialog::onCutoffChanged);
  connect(ui->cutoffSlider, qOverload<int>(&QSlider::valueChanged), this, &ImageTraceDialog::onCutoffChanged);
  connect(ui->thresholdSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &ImageTraceDialog::onThresholdChanged);
  connect(ui->thresholdSlider, qOverload<int>(&QSlider::valueChanged), this, &ImageTraceDialog::onThresholdChanged);
  connect(ui->ignoreSpinBox, qOverload<int>(&QSpinBox::valueChanged), [=](int ignore_val) {
    this->updateImageTrace();
  });
  connect(ui->smoothnessDoubleSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), [=](qreal smoothness) {
    this->updateImageTrace();
  });
  connect(ui->optimizeDoubleSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), [=](qreal optimize) {
    this->updateImageTrace();
  });

  connect(ui->bgImageComboBox, qOverload<int>(&QComboBox::currentIndexChanged), [=](int new_index) {
    this->updateBackgroundDisplay();
    this->updateImageTrace();
  });

  connect(ui->selectPartialCheckBox, &QCheckBox::stateChanged, this, &ImageTraceDialog::onSelectPartialStateChanged);
}

void ImageTraceDialog::loadImage(const QImage *img) {
  ui->traceGraphicsView->setImage(ImageToGrayscale(*img));
  resetParams();
  try {
    this->updateBackgroundDisplay();
    this->updateImageTrace();
  } catch (const std::exception& e) {
    qInfo() << e.what();
    return;
  }
}

QImage ImageTraceDialog::ImageToGrayscale(const QImage &image)
{
  QImage result_img = image.convertToFormat(QImage::Format_Grayscale8);

  bool apply_alpha = image.hasAlphaChannel();
  if (apply_alpha) {
    for (int y = 0; y < image.height(); ++y) {
      for (int x = 0; x < image.width(); ++x) {
        if (qAlpha(image.pixel(x, y)) == 0) { // transparent -> consider as white
          result_img.setPixel(x, y, qRgb(255, 255, 255));
        } else {
          // apply alpha (opacity)
          int gray_val = 255 - (255 - qGray(result_img.pixel(x, y))) * qAlpha(image.pixel(x, y)) / 255;
          result_img.setPixel(x, y, qRgb(gray_val, gray_val, gray_val));
        }
      }
    }
  }
  return result_img;
}

void ImageTraceDialog::updateBackgroundDisplay() {
  if (ui->bgImageComboBox->currentIndex() == 0) {
    ui->traceGraphicsView->updateBackgroundImage(ImageTraceGraphicsView::kGrayscale);
  } else if (ui->bgImageComboBox->currentIndex() == 1) {
    ui->traceGraphicsView->updateBackgroundImage(ImageTraceGraphicsView::kBinarized);
  } else {
    ui->traceGraphicsView->updateBackgroundImage(ImageTraceGraphicsView::kFaded);
  }
}

void ImageTraceDialog::updateImageTrace() {
  ui->traceGraphicsView->updateTrace(ui->cutoffSpinBox->value(), ui->thresholdSpinBox->value(),
                                     ui->ignoreSpinBox->value(), ui->smoothnessDoubleSpinBox->value(),
                                     ui->optimizeDoubleSpinBox->value());
}
