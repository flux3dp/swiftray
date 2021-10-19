#include "image-sharpen-dialog.h"
#include "ui_image-sharpen-dialog.h"
#include <QPushButton>

#include <memory>
#include <QDebug>
#include <QGraphicsView>
#include <QPixmap>
#include <QOpenGLWidget>
#include <QSurfaceFormat>

QImage cvMatToQImage (const cv::Mat &inMat) {
  switch (inMat.type()) {
    case CV_8UC4: {
      QImage image(inMat.data, inMat.cols, inMat.rows, static_cast<int>(inMat.step), QImage::Format_ARGB32);
      return image;
    }
    case CV_8UC3: {
      QImage image(inMat.data, inMat.cols, inMat.rows, static_cast<int>(inMat.step), QImage::Format_RGB888);
      return image.rgbSwapped();
    }
    case CV_8UC1: {
      QImage image(inMat.data, inMat.cols, inMat.rows, static_cast<int>(inMat.step), QImage::Format_Grayscale8);
      return image;
    }
    default:
      qWarning() << "cvMatToQImage() failed, the type of mat is: " << inMat.type();
      break;
  }

  return QImage();
}

cv::Mat QImageToCvMat(const QImage &inImage) {
  switch (inImage.format()) {
    case QImage::Format_ARGB32: {
      cv::Mat mat(inImage.height(), inImage.width(),
                  CV_8UC4,
                  const_cast<uchar*>(inImage.bits()),
                  static_cast<size_t>(inImage.bytesPerLine())
      );
      return mat;
    }
    case QImage::Format_RGB888: {
      QImage swapped = inImage;

      swapped = swapped.rgbSwapped();

      return (cv::Mat (swapped.height(), swapped.width(),
                  CV_8UC4,
                  const_cast<uchar*>(swapped.bits()),
                  static_cast<size_t>(swapped.bytesPerLine())).clone()
      );
    }
    case QImage::Format_Grayscale8: {
      cv::Mat mat(inImage.height(), inImage.width(),
                  CV_8UC1,
                  const_cast<uchar*>(inImage.bits()),
                  static_cast<size_t>(inImage.bytesPerLine())
      );
      return mat;
    }
    default:
      qWarning() << "QImageToCvMat() failed, the QImage format is: " << inImage.format();
  }

  return cv::Mat();
}


ImageSharpenDialog::ImageSharpenDialog(QWidget *parent) :
        QDialog(parent), ui(new Ui::ImageSharpenDialog) {
  ui->setupUi(this);

  // Use OpenGL for better performance
  QOpenGLWidget *gl = new QOpenGLWidget();
  QSurfaceFormat format;
  format.setSamples(1);
  gl->setFormat(format);
  ui->sharpenGraphicsView->setViewport(gl);

  initializeContainer();

  reset();
}

ImageSharpenDialog::~ImageSharpenDialog() {
  delete ui;
}

void ImageSharpenDialog::reset() {
  ui->sharpenGraphicsView->reset();
  src_image_grayscale_ = QImage();

  // Reset GUI inputs
  resetParams();
}

/**
 * @brief reset sharpen params
 */
void ImageSharpenDialog::resetParams() {
  setSharpnessSliderWithoutEmit(0);
  setSharpnessSpinboxWithoutEmit(0);
  setRadiusSliderWithoutEmit(0);
  setRadiusSpinboxWithoutEmit(0);
}

void ImageSharpenDialog::onSharpnessChanged(int new_sharpness_val) {
  setSharpnessSliderWithoutEmit(new_sharpness_val);
  setSharpnessSpinboxWithoutEmit(new_sharpness_val);
  updateImageSharpen();
}

void ImageSharpenDialog::onRadiusChanged(int new_radius_val) {
  setRadiusSpinboxWithoutEmit(new_radius_val);
  setRadiusSliderWithoutEmit(new_radius_val);
  updateImageSharpen();
}

void ImageSharpenDialog::registerEvents() {
  connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked,
          this, &ImageSharpenDialog::accept);
  connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked,
          this, &ImageSharpenDialog::reject);

  connect(ui->sharpnessSpinBox, qOverload<int>(&QSpinBox::valueChanged),
          this, &ImageSharpenDialog::onSharpnessChanged);
  connect(ui->sharpnessSlider, qOverload<int>(&QSlider::valueChanged),
          this, &ImageSharpenDialog::onSharpnessChanged);
  connect(ui->radiusSpinBox, qOverload<int>(&QSpinBox::valueChanged),
          this, &ImageSharpenDialog::onRadiusChanged);
  connect(ui->radiusSlider, qOverload<int>(&QSlider::valueChanged),
          this, &ImageSharpenDialog::onRadiusChanged);
}

void ImageSharpenDialog::loadStyles() {
  ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Ok"));
  ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
}

/**
 * @brief load a new image and generate a sharpened image
 *        might need a reset() before
 * @param img could be rgb/rgba image
 *            converted into grayscale when loaded
 */
void ImageSharpenDialog::loadImage(const QImage &img) {
  src_image_grayscale_ = ImageToGrayscale(img);
  try {
    ui->sharpenGraphicsView->scene()->setSceneRect(0, 0,
                                                 src_image_grayscale_.width(),
                                                 src_image_grayscale_.height());
    updateBackgroundDisplay();
    updateImageSharpen();
  } catch (const std::exception& e) {
    qInfo() << e.what();
    return;
  }
}


/**
 * @brief Convert rgb/rgba image to grayscale image and also keep alpha channel
 * @param image source image
 * @return RGBA32/Grascale8 image
 */
QImage ImageSharpenDialog::ImageToGrayscale(const QImage &image)
{
  QImage result_img;
  bool apply_alpha = image.hasAlphaChannel();
  if (apply_alpha) {
    result_img = image.convertToFormat(QImage::Format_ARGB32);
  } else {
    result_img = image.convertToFormat(QImage::Format_Grayscale8);
  }

  if (apply_alpha) {
    for (int y = 0; y < result_img.height(); ++y) {
      for (int x = 0; x < result_img.width(); ++x) {
        QRgb pixel = result_img.pixel(x, y);
        uint ci = uint(qGray(pixel));
        result_img.setPixel(x, y, qRgba(ci, ci, ci, qAlpha(pixel)));
      }
    }
  }
  return result_img;
}

/**
 * @brief Fade rgb/rgba image
 * @param image source image
 * @return RGBA32/Grascale8 image
 */
QImage ImageSharpenDialog::FadeImage(const QImage &image)
{
  QImage result_img = image;
  bool apply_alpha = image.hasAlphaChannel();
  if (apply_alpha) {
    if (image.format() != QImage::Format_ARGB32) {
      result_img = image.convertToFormat(QImage::Format_ARGB32);
    }
  } else {
    if (image.format() != QImage::Format_Grayscale8) {
      result_img = image.convertToFormat(QImage::Format_Grayscale8);
    }
  }

  //if (apply_alpha) {
    for (int y = 0; y < result_img.height(); ++y) {
      for (int x = 0; x < result_img.width(); ++x) {
        QRgb pixel = result_img.pixel(x, y);
        uint ci = uint(qGray(pixel));
        ci = (ci + 50) > 255 ? 255 : (ci + 50);
        if (apply_alpha) {
          if (qAlpha(pixel) != 0) {
            result_img.setPixel(x, y, qRgba(ci, ci, ci, qAlpha(pixel)));
          }
        } else {
            result_img.setPixel(x, y, qRgba(ci, ci, ci, qAlpha(pixel)));
        }
      }
    }
  //} else {  
  //}
  return result_img;
}

/**
 * @brief Create background pixmap based on background mode
 *        and update it on the graphicsscene
 */
void ImageSharpenDialog::updateBackgroundDisplay() {
  if (src_image_grayscale_.isNull()) {
    return;
  }

  ui->sharpenGraphicsView->updateBackgroundPixmap(
    QPixmap::fromImage(
      FadeImage(src_image_grayscale_)
    )
  );
}

/**
 * @brief Generate a sharpened image
 *        and update it on graphicscene
 */
void ImageSharpenDialog::updateImageSharpen() {
  try {
    if (src_image_grayscale_.isNull()) {
      return;
    }
    cv::Mat image = QImageToCvMat(src_image_grayscale_);
    cv::Mat image_blurred_with_radius_kernel;
    cv::Mat dst;
    int ksize = 2 * ui->radiusSpinBox->value() + 1;
    cv::GaussianBlur(image, image_blurred_with_radius_kernel, cv::Size(ksize, ksize), 0);
    cv::addWeighted(image, 1 + ui->sharpnessSpinBox->value(), image_blurred_with_radius_kernel, -ui->sharpnessSpinBox->value(), 0, dst);
    sharpened_image_ = cvMatToQImage(dst);

    ui->sharpenGraphicsView->updateBackgroundPixmap(
      QPixmap::fromImage(
        sharpened_image_
      )
    );

  } catch (const std::exception& e) {
    qInfo() << e.what();
    return;
  }
}


void ImageSharpenDialog::setSharpnessSpinboxWithoutEmit(int sharpness) {
  ui->sharpnessSpinBox->blockSignals(true);
  ui->sharpnessSpinBox->setValue(sharpness);
  ui->sharpnessSpinBox->blockSignals(false);
}

void ImageSharpenDialog::setSharpnessSliderWithoutEmit(int sharpness) {
  ui->sharpnessSlider->blockSignals(true);
  ui->sharpnessSlider->setValue(sharpness);
  ui->sharpnessSlider->blockSignals(false);
}

void ImageSharpenDialog::setRadiusSpinboxWithoutEmit(int radius) {
  ui->radiusSpinBox->blockSignals(true);
  ui->radiusSpinBox->setValue(radius);
  ui->radiusSpinBox->blockSignals(false);
}

void ImageSharpenDialog::setRadiusSliderWithoutEmit(int radius) {
  ui->radiusSlider->blockSignals(true);
  ui->radiusSlider->setValue(radius);
  ui->radiusSlider->blockSignals(false);
}
