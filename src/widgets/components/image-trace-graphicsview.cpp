#include <widgets/components/image-trace-graphicsview.h>
#include <widgets/components/base-graphicsview.h>
#include <QScrollBar>
#include <QPainterPath>
#include <QxPotrace/include/qxpotrace.h>
#include <QGraphicsRectitem>

#include <QDebug>

ImageTraceGraphicsView::ImageTraceGraphicsView(QWidget *parent)
        : BaseGraphicsView(parent)
{
  potrace_ = std::make_shared<QxPotrace>();
}

ImageTraceGraphicsView::ImageTraceGraphicsView(QGraphicsScene *scene, QWidget *parent)
        : BaseGraphicsView(scene, parent)
{
  potrace_ = std::make_shared<QxPotrace>();
}

void ImageTraceGraphicsView::setImage(QImage src_img) {
  src_image_grayscale_ = src_img;
}

void ImageTraceGraphicsView::mousePressEvent(QMouseEvent *event) {
  if (dragMode() == RubberBandDrag) {
    QPainterPath empty_select;
    this->scene()->setSelectionArea(empty_select);
    emit rubberBandSelect(QRectF()); // Reset rubberband
  }
  BaseGraphicsView::mousePressEvent(event);
}


void ImageTraceGraphicsView::mouseReleaseEvent(QMouseEvent *event) {
  if (dragMode() == RubberBandDrag) {
    this->scene()->selectionArea().boundingRect();
    emit rubberBandSelect(this->scene()->selectionArea().boundingRect());
  }
  BaseGraphicsView::mouseReleaseEvent(event);
}






void ImageTraceGraphicsView::updateTrace(int cutoff, int threshold, int turd_size, qreal smooth, qreal optimize) {
  try {
    if (selection_area_rect_item_) {
      this->scene()->removeItem(selection_area_rect_item_);
      selection_area_rect_item_ = nullptr;
    }
    if (selection_area_pixmap_item_) {
      this->scene()->removeItem(selection_area_pixmap_item_);
      selection_area_pixmap_item_ = nullptr;
    }
    QRectF partial_select = this->scene()->selectionArea().boundingRect();
    if (!(partial_select.size().toSize() == QSize(0, 0))) {
      selection_area_rect_item_ = this->scene()->addRect(partial_select, QPen(QColor(Qt::red)));
      QImage subimg = createSubImage(&src_image_grayscale_, partial_select.toRect());
      subimg.invertPixels(QImage::InvertRgb);
      selection_area_pixmap_item_ = this->scene()->addPixmap(QPixmap::fromImage(subimg));
      selection_area_pixmap_item_->setOffset(partial_select.topLeft().x(), partial_select.topLeft().y());
    } else {
      if (!potrace_->trace(src_image_grayscale_,
                           cutoff, threshold, turd_size, smooth, optimize)) {
        qInfo() << "Error occurred when generating trace";
        return;
      }
    }

    if (contours_path_item_) {
      this->scene()->removeItem(this->contours_path_item_);
      contours_path_item_ = nullptr;
    }
    this->contours_path_item_ = this->scene()->addPath(potrace_->getContours(), QPen{Qt::green});

  } catch (const std::exception& e) {
    qInfo() << e.what();
    return;
  }
}

void ImageTraceGraphicsView::updateBackgroundImage(BackgroundDisplayMode mode) {
  // Convert loaded image to grayscale (or binarize) and add to GraphicsScene as background
  if (bg_image_item_) {
    this->scene()->removeItem(bg_image_item_);
    bg_image_item_ = nullptr;
  }
  if (mode == BackgroundDisplayMode::kGrayscale) {
    // Grayscale
    bg_image_item_ = new QGraphicsPixmapItem(QPixmap::fromImage(src_image_grayscale_));
  } else if (mode == BackgroundDisplayMode::kBinarized) {
    // Binarized
    bg_image_item_ = new QGraphicsPixmapItem(QPixmap::fromImage(ImageBinarize(src_image_grayscale_, high_thres_, low_thres_)));
  } else {
    // TODO: Faded
    bg_image_item_ = new QGraphicsPixmapItem(QPixmap::fromImage(src_image_grayscale_));
  }
  this->scene()->addItem(bg_image_item_);
}

QImage ImageTraceGraphicsView::ImageBinarize(const QImage &image, int threshold, int cutoff)
{
  QImage result_img{image.width(), image.height(), QImage::Format_Grayscale8};

  for (int y = 0; y < image.height(); ++y) {
    for (int x = 0; x < image.width(); ++x) {
      if (qGray(image.pixel(x, y)) < cutoff) {
        result_img.setPixel(x, y, qRgb(255, 255, 255));
      } else if (qGray(image.pixel(x, y)) > threshold) {
        result_img.setPixel(x, y, qRgb(255, 255, 255));
      } else {
        result_img.setPixel(x, y, qRgb(0, 0, 0));
      }
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
QImage ImageTraceGraphicsView::createSubImage(QImage* image, const QRect & rect) {
  return image->copy(rect.topLeft().x(), rect.topLeft().y(), rect.width(), rect.height());
}