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
  auto item = getSelectionAreaRectItem();
  if (item) {
    scene()->removeItem(item);
  }
}

/**
 * @brief clear and draw new background image
 * @param background_pixmap
 */
void ImageTraceGraphicsView::updateBackgroundPixmap(QPixmap background_pixmap) {
  auto item = getBackgroundPixmapItem();
  if (item) {
    item->setPixmap(background_pixmap);
  } else {
    // If not exist -> create and add a new graphics item
    item = this->scene()->addPixmap(background_pixmap);
    item->setData(ITEM_ID_KEY, BACKGROUND_IMAGE_ITEM_ID);
    item->setZValue(BACKGROUND_IMAGE_Z_INDEX); // overlapped by any other items
  }
}

/**
 * @brief clear and draw new trace contours
 * @param contours
 */
void ImageTraceGraphicsView::updateTrace(const QPainterPath& contours) {
  auto item = getTraceContourPathsItem();
  if (item) {
    item->setPath(contours);
  } else {
    item = this->scene()->addPath(contours, QPen{Qt::green});
    item->setData(ITEM_ID_KEY, IMAGE_TRACE_ITEM_ID);
    item->setZValue(IMAGE_TRACE_Z_INDEX); // top-most
  }
}

/**
 * @brief clear and draw anchor points on contours
 *        middle points of bezier (cubic curve) are ignored
 * @param contours
 * @param show_points
 */
void ImageTraceGraphicsView::updateAnchorPoints(const QPainterPath& contours, bool show_points) {
  // Clear points shown on scene
  auto anchor_item = getTraceContourAnchorItem();
  while (anchor_item) {
    scene()->removeItem(anchor_item);
    anchor_item = getTraceContourAnchorItem();
  }

  QPen pen{Qt::green, 1};
  QBrush brush{QColor{Qt::green}};
  pen.setCosmetic(true);
  if (show_points) {
    for (auto i = 0; i < contours.elementCount(); i++) {
      QGraphicsEllipseItem* anchor_item;
      if (contours.elementAt(i).isCurveTo()) {
        // curveTo point is the first middle point of cubic curve
        // For cubic curve, there are one curveTo and one moveTo middle point
        // they both aren't on the path -> ignore
      } else {
        anchor_item = scene()->addEllipse(
                QPointF(contours.elementAt(i)).x() - 1,
                QPointF(contours.elementAt(i)).y() - 1, 2, 2, pen, brush);
        anchor_item->setData(ITEM_ID_KEY, ANCHOR_POINTS_ITEM_ID);
        anchor_item->setZValue(ANCHOR_POINTS_Z_INDEX);
        if (contours.elementAt(i+1).isCurveTo()) {
          // WARNING: MUST ensure contours only contain cubic curves and lines!
          i += 2; // ignore two bezier middle points (not on the path)
        }
      }
    }
  }
}

/**
 * @brief clear old and draw new selection area
 */
void ImageTraceGraphicsView::drawSelectionArea() {
  auto item = getSelectionAreaRectItem();

  // Only draw when selection area exists (nonzero)
  if (this->scene()->selectionArea().boundingRect().size().toSize() != QSize(0, 0)) {
    if (item) {
      item->setRect(scene()->selectionArea().boundingRect());
    } else {
      item = scene()->addRect(
              scene()->selectionArea().boundingRect(),
              QPen(QColor(Qt::red)));
      item->setData(ITEM_ID_KEY, SELECTION_RECT_ITEM_ID);
      item->setZValue(SELECTION_RECT_Z_INDEX); // on top of background image but under trace contour
    }
  } else {
    if (item) {
      scene()->removeItem(item);
    }
  }
}

QGraphicsPixmapItem* ImageTraceGraphicsView::getBackgroundPixmapItem() {
  for (auto item: scene()->items()) {
    if (item->data(ITEM_ID_KEY).toString().compare(QString(BACKGROUND_IMAGE_ITEM_ID)) == 0 &&
        qgraphicsitem_cast<QGraphicsPixmapItem *>(item)) {
      return qgraphicsitem_cast<QGraphicsPixmapItem *>(item);
    }
  }
  return nullptr;
}

QGraphicsRectItem* ImageTraceGraphicsView::getSelectionAreaRectItem() {
  for (auto item: scene()->items()) {
    if (item->data(ITEM_ID_KEY).toString().compare(QString(SELECTION_RECT_ITEM_ID)) == 0 &&
        qgraphicsitem_cast<QGraphicsRectItem *>(item)) {
      return qgraphicsitem_cast<QGraphicsRectItem *>(item);
    }
  }
  return nullptr;
}

QGraphicsPathItem* ImageTraceGraphicsView::getTraceContourPathsItem() {
  for (auto item: scene()->items()) {
    if (item->data(ITEM_ID_KEY).toString().compare(QString(IMAGE_TRACE_ITEM_ID)) == 0 &&
        qgraphicsitem_cast<QGraphicsPathItem *>(item)) {
      return qgraphicsitem_cast<QGraphicsPathItem *>(item);
    }
  }
  return nullptr;
}

QGraphicsEllipseItem* ImageTraceGraphicsView::getTraceContourAnchorItem() {
  for (auto item: scene()->items()) {
    if (item->data(ITEM_ID_KEY).toString().compare(QString(ANCHOR_POINTS_ITEM_ID)) == 0 &&
        qgraphicsitem_cast<QGraphicsEllipseItem *>(item)) {
      return qgraphicsitem_cast<QGraphicsEllipseItem *>(item);
    }
  }
  return nullptr;
}
