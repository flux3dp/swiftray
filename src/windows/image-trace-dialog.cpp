#include "image-trace-dialog.h"
#include "ui_image-trace-dialog.h"
#include <QPushButton>

#include <memory>
#include <QDebug>
#include <QGraphicsView>
#include <QPixmap>
#include <widgets/components/image-trace-graphicsview.h>
#include <QxPotrace/include/qxpotrace.h>


ImageTraceDialog::ImageTraceDialog(QWidget *parent) :
        QDialog(parent), ui(new Ui::ImageTraceDialog) {
  ui->setupUi(this);

  registerEvents();

  reset();
}

ImageTraceDialog::~ImageTraceDialog() {
  delete ui;
}

void ImageTraceDialog::reset() {

  ui->traceGraphicsView->reset();
  src_image_grayscale_ = QImage();
  potrace_ = std::make_shared<QxPotrace>();

  // Reset GUI inputs
  resetParams();
  ui->bgImageComboBox->setCurrentIndex(0);
  ui->selectPartialCheckBox->setCheckState(Qt::Unchecked);
}

/**
 * @brief reset Potrace params
 */
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
  if (ui->bgImageComboBox->currentIndex() == 1) {
    updateBackgroundDisplay();
  }
  updateImageTrace();
}

void ImageTraceDialog::onSelectPartialStateChanged(int check_state) {
  ui->traceGraphicsView->clearSelectionArea();
  if (check_state == Qt::Checked) {
    ui->traceGraphicsView->setDragMode(QGraphicsView::RubberBandDrag);
  } else if (check_state == Qt::Unchecked) {
    ui->traceGraphicsView->setDragMode(QGraphicsView::ScrollHandDrag);
  }
  updateImageTrace();
}

void ImageTraceDialog::registerEvents() {
  connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, [=](){
    // TODO: Convert trace paths to shape on Canvas

    this->reset();
    this->close();
  });
  connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, [=]() {
    this->reset();
    this->close();
  });

  connect(ui->cutoffSpinBox, qOverload<int>(&QSpinBox::valueChanged),
          this, &ImageTraceDialog::onCutoffChanged);
  connect(ui->cutoffSlider, qOverload<int>(&QSlider::valueChanged),
          this, &ImageTraceDialog::onCutoffChanged);
  connect(ui->thresholdSpinBox, qOverload<int>(&QSpinBox::valueChanged),
          this, &ImageTraceDialog::onThresholdChanged);
  connect(ui->thresholdSlider, qOverload<int>(&QSlider::valueChanged),
          this, &ImageTraceDialog::onThresholdChanged);
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
  });

  connect(ui->selectPartialCheckBox, &QCheckBox::stateChanged,
          this, &ImageTraceDialog::onSelectPartialStateChanged);
  connect(ui->traceGraphicsView, &ImageTraceGraphicsView::selectionAreaChanged, this, [=]() {
    this->updateImageTrace();
  });
}

/**
 * @brief load a new image and generate a trace
 *        might need a reset() before
 * @param img could be rgb/rgba image
 *            converted into grayscale when loaded
 */
void ImageTraceDialog::loadImage(const QImage *img) {
  src_image_grayscale_ = ImageToGrayscale(*img);
  try {
    ui->traceGraphicsView->scene()->setSceneRect(0, 0,
                                                 src_image_grayscale_.width(),
                                                 src_image_grayscale_.height());
    updateBackgroundDisplay();
    updateImageTrace();
  } catch (const std::exception& e) {
    qInfo() << e.what();
    return;
  }
}

/**
 * @brief   Convert rgb/rgba image to grayscale 8 image
 *          NOTE: transparent area (alpha = 0) -> white (255)
 * @param image source image
 * @return
 */
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

/**
 * @brief convert image to a image containing only 0/255 pixel value
 *        when image pixel value < low threshold or > high threshold -> 0
 *        when image pixel value >= low threshold and <= high threshold -> 1
 * @param image source image
 * @param threshold high threshold for pixel value
 * @param cutoff low threshold for pixel value
 * @return
 */
QImage ImageTraceDialog::ImageBinarize(const QImage &image, int threshold, int cutoff)
{
  QImage result_img{image.width(), image.height(), QImage::Format_Grayscale8};

  for (int y = 0; y < image.height(); ++y) {
    for (int x = 0; x < image.width(); ++x) {
      result_img.setPixel(x, y,
               qGray(image.pixel(x, y)) < cutoff ? qRgb(255, 255, 255) :
                              qGray(image.pixel(x, y)) <= threshold ? qRgb(0, 0, 0) :
                                  qRgb(255, 255, 255));
    }
  }
  return result_img;
}

/**
 * @brief Select partial image and return as a new QImage
 * @param image
 * @param rect
 * @return
 */
QImage ImageTraceDialog::createSubImage(QImage* image, const QRect & rect) {
  return image->copy(rect.left(), rect.top(), rect.width(), rect.height());
}

/**
 * @brief Create background pixmap based on background mode
 *        and update it on the graphicsscene
 */
void ImageTraceDialog::updateBackgroundDisplay() {
  if (src_image_grayscale_.isNull()) {
    return;
  }

  // Convert loaded image to grayscale/binarize/faded
  // and then add to GraphicsScene as background
  if (ui->bgImageComboBox->currentIndex() == 0) { // grayscale
    ui->traceGraphicsView->updateBackgroundPixmap(
            QPixmap::fromImage(src_image_grayscale_));
  } else if (ui->bgImageComboBox->currentIndex() == 1) { // binarized
    ui->traceGraphicsView->updateBackgroundPixmap(
QPixmap::fromImage(
          ImageBinarize(src_image_grayscale_,
                      ui->thresholdSpinBox->value(),
                        ui->cutoffSpinBox->value()
                )
              )
    );
  } else { // faded
    ui->traceGraphicsView->updateBackgroundPixmap(
            QPixmap::fromImage(src_image_grayscale_));
  }
}

/**
 * @brief Generate image trace contours (partial or full)
 *        and update it on graphicscene
 */
void ImageTraceDialog::updateImageTrace() {
  try {
    if (src_image_grayscale_.isNull()) {
      return;
    }

    QRectF select_area = ui->traceGraphicsView->scene()->selectionArea().boundingRect();
    QPointF offset(0, 0);
    if (select_area.size().toSize() != QSize(0, 0)) {
      // intersect of image and selection area on scene
      QRectF intersect_area = ui->traceGraphicsView->scene()->selectionArea().boundingRect().intersected(src_image_grayscale_.rect());
      offset = intersect_area.topLeft();
      QImage sub_img = createSubImage(&src_image_grayscale_, intersect_area.toRect());
      if (!potrace_->trace(sub_img,
                           ui->cutoffSpinBox->value(),
                           ui->thresholdSpinBox->value(),
                           ui->ignoreSpinBox->value(),
                           ui->smoothnessDoubleSpinBox->value(),
                           ui->optimizeDoubleSpinBox->value())) {
        qInfo() << "Error occurred when generating trace";
        return;
      }
    } else {
      if (!potrace_->trace(src_image_grayscale_,
                           ui->cutoffSpinBox->value(),
                           ui->thresholdSpinBox->value(),
                           ui->ignoreSpinBox->value(),
                           ui->smoothnessDoubleSpinBox->value(),
                           ui->optimizeDoubleSpinBox->value())) {
        qInfo() << "Error occurred when generating trace";
        return;
      }
    }

    QPainterPath contours = potrace_->getContours();
    contours.translate(offset.x(), offset.y());
    ui->traceGraphicsView->updateTrace(contours);

  } catch (const std::exception& e) {
    qInfo() << e.what();
    return;
  }
}
