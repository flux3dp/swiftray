#include <widgets/components/image-trace-graphicsview.h>
#include <widgets/components/base-graphicsview.h>
#include <QScrollBar>
#include <QPainterPath>
#include <QGraphicsRectitem>

#include <QDebug>

ImageTraceGraphicsView::ImageTraceGraphicsView(QWidget *parent)
        : BaseGraphicsView(parent)
{
  min_scale_ = 0.5;
  QGraphicsScene* new_scene = new QGraphicsScene();
  setScene(new_scene);
}

ImageTraceGraphicsView::ImageTraceGraphicsView(QGraphicsScene *scene, QWidget *parent)
        : BaseGraphicsView(scene, parent)
{
  min_scale_ = 0.5;
  QGraphicsScene* new_scene = new QGraphicsScene();
  setScene(new_scene);
}

void ImageTraceGraphicsView::reset() {
  //this->clearSelectionArea();
  bg_image_item_ = nullptr;
  contours_path_item_ = nullptr;
  selection_area_rect_item_ = nullptr;
  QGraphicsScene* new_scene = new QGraphicsScene();
  this->setScene(new_scene);
  this->resetTransform();
}

void ImageTraceGraphicsView::mousePressEvent(QMouseEvent *event) {
  if (dragMode() == RubberBandDrag) {
    clearSelectionArea();
  }
  BaseGraphicsView::mousePressEvent(event);
}


void ImageTraceGraphicsView::mouseReleaseEvent(QMouseEvent *event) {
  if (dragMode() == RubberBandDrag) {
    // TODO: Handle select partial image for image trace
    drawSelectionArea();
    emit selectionAreaChanged();
  }
  BaseGraphicsView::mouseReleaseEvent(event);
}

/**
 * @brief clear selectionArea of scene (for unknown reason clearSelectionArea doesn't work)
 *        if exist, release visual selectionArea rect shown on scene
 */
void ImageTraceGraphicsView::clearSelectionArea() {
  QPainterPath empty_path;
  this->scene()->setSelectionArea(empty_path);
  if (selection_area_rect_item_ && selection_area_rect_item_->scene()) {
    selection_area_rect_item_->scene()->removeItem(selection_area_rect_item_);
  }
  selection_area_rect_item_ = nullptr;
}

/**
 * @brief clear and draw new background image
 * @param background_pixmap
 */
void ImageTraceGraphicsView::updateBackgroundPixmap(QPixmap background_pixmap) {
  if (bg_image_item_ && bg_image_item_->scene()) {
    this->scene()->removeItem(bg_image_item_);
  }
  bg_image_item_ = nullptr;
  bg_image_item_ = new QGraphicsPixmapItem(background_pixmap);
  bg_image_item_->setZValue(BACKGROUND_IMAGE_Z_INDEX); // overlapped by any other items
  this->scene()->addItem(bg_image_item_);
}

/**
 * @brief clear and draw new trace contours
 * @param contours
 */
void ImageTraceGraphicsView::updateTrace(QPainterPath contours) {
  if (contours_path_item_ && contours_path_item_->scene()) {
    contours_path_item_->scene()->removeItem(contours_path_item_);
  }
  contours_path_item_ = nullptr;
  this->contours_path_item_ = this->scene()->addPath(contours, QPen{Qt::green});
  this->contours_path_item_->setZValue(IMAGE_TRACE_Z_INDEX); // top-most
}

/**
 * @brief clear old and draw new selection area
 */
void ImageTraceGraphicsView::drawSelectionArea() {
  if (selection_area_rect_item_ && selection_area_rect_item_->scene()) {
    selection_area_rect_item_->scene()->removeItem(selection_area_rect_item_);
  }
  selection_area_rect_item_ = nullptr;
  // Only draw when selection area exists (nonzero)
  if (this->scene()->selectionArea().boundingRect().size().toSize() != QSize(0, 0)) {
    selection_area_rect_item_ = this->scene()->addRect(
            this->scene()->selectionArea().boundingRect(),
            QPen(QColor(Qt::red)));
    selection_area_rect_item_->setZValue(SELECTION_RECT_Z_INDEX); // on top of background image but under trace contour
  }
}

