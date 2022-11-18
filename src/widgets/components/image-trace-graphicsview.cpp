#include <widgets/components/image-trace-graphicsview.h>
#include <widgets/components/base-graphicsview.h>
#include <QScrollBar>
#include <QPainterPath>
#include <QGraphicsRectitem>

#include <QDebug>

ImageTraceGraphicsView::QGraphicsContourPathsItem::QGraphicsContourPathsItem():
        QGraphicsPathItem() {
}

ImageTraceGraphicsView::QGraphicsContourPathsItem::QGraphicsContourPathsItem(
        const QPainterPath &path, QGraphicsItem *parent):
        QGraphicsPathItem(path, parent) {
}

ImageTraceGraphicsView::QGraphicsContourPathsItem::QGraphicsContourPathsItem(
        QGraphicsItem *parent):
        QGraphicsPathItem(parent){
}

QRectF ImageTraceGraphicsView::QGraphicsContourPathsItem::boundingRect() const {
  return QGraphicsPathItem::boundingRect();
}

void ImageTraceGraphicsView::QGraphicsContourPathsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {

  // Draw trace contours
  QPen pen{Qt::green, 2};
  pen.setCosmetic(true);
  setPen(pen);
  QGraphicsPathItem::paint(painter, option, widget);

  // Draw endpoints on contours
  if (show_points_) {
    QBrush brush{QColor{Qt::green}};
    painter->setBrush(brush);
    for (auto i = 0; i < path().elementCount(); i++) {
      if (path().elementAt(i).isCurveTo()) {
        // curveTo point is the first middle point of cubic curve
        // For cubic curve, there are one curveTo and one moveTo middle point
        // they both aren't on the path -> ignore
      } else {
        QPointF p = QPointF(path().elementAt(i));
        // Scale the size of circle points based on scale of GraphicsView to maintain a fixed size
        painter->drawEllipse(QRectF(
                p.x() - (5.0 / qAbs(painter->deviceTransform().m22())),
                p.y() - (5.0 / qAbs(painter->deviceTransform().m22())),
                10.0 / qAbs(painter->deviceTransform().m22()),
                10.0 / qAbs(painter->deviceTransform().m22()))
        );
        if (path().elementAt(i+1).isCurveTo()) {
          // WARNING: MUST ensure contours only contain cubic curves and lines!
          i += 2; // ignore two bezier middle points (not on the path)
        }
      }
    }
  }
}


ImageTraceGraphicsView::ImageTraceGraphicsView(QWidget *parent)
        : BaseGraphicsView(parent)
{
  setMinScale(0.5);
}

void ImageTraceGraphicsView::reset() {
  QGraphicsScene* new_scene = new QGraphicsScene();
  setScene(new_scene);
  resetTransform();
}

void ImageTraceGraphicsView::mousePressEvent(QMouseEvent *event) {
  if (dragMode() == RubberBandDrag) {
    clearSelectionArea();
  }
  BaseGraphicsView::mousePressEvent(event);
}

void ImageTraceGraphicsView::mouseReleaseEvent(QMouseEvent *event) {
  if (dragMode() == RubberBandDrag) {
    drawSelectionArea();
    Q_EMIT selectionAreaChanged();
  }
  BaseGraphicsView::mouseReleaseEvent(event);
}

/**
 * @brief clear selectionArea of scene (for unknown reason clearSelectionArea doesn't work)
 *        if exist, release visual selectionArea rect shown on scene
 */
void ImageTraceGraphicsView::clearSelectionArea() {
  QPainterPath empty_path;
  scene()->setSelectionArea(empty_path);
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
    item = scene()->addPixmap(background_pixmap);
    item->setData(ITEM_ID_KEY, BACKGROUND_IMAGE_ITEM_ID);
    item->setZValue(BACKGROUND_IMAGE_Z_INDEX); // overlapped by any other items
  }
}

/**
 * @brief clear and draw new trace contours
 * @param contours
 */
void ImageTraceGraphicsView::updateTrace(const QPainterPath& contours) {
  auto contour_item = getTraceContourPathsItem();
  if (contour_item) {
    contour_item->setPath(contours);
  } else {
    contour_item = new QGraphicsContourPathsItem(contours);
    contour_item->setPen(QPen(Qt::yellow, 2));
    scene()->addItem(contour_item);
    contour_item->setData(ITEM_ID_KEY, IMAGE_TRACE_ITEM_ID);
    contour_item->setZValue(IMAGE_TRACE_Z_INDEX); // top-most
  }
}

/**
 * @brief switch on/off display of endpoints on trace contours
 * @param enable
 */
void ImageTraceGraphicsView::setShowPoints(bool enable) {
  auto contour_item = getTraceContourPathsItem();
  if (contour_item) {
    contour_item->setShowPoints(enable);
    contour_item->update();
  }
}

/**
 * @brief clear old and draw new selection area
 */
void ImageTraceGraphicsView::drawSelectionArea() {
  auto item = getSelectionAreaRectItem();

  // Only draw when selection area exists (nonzero)
  if (scene()->selectionArea().boundingRect().size().toSize() != QSize(0, 0)) {
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

QGraphicsRectItem* ImageTraceGraphicsView::getSelectionAreaRectItem() {
  for (auto item: scene()->items()) {
    if (item->data(ITEM_ID_KEY).toString().compare(QString(SELECTION_RECT_ITEM_ID)) == 0 &&
        qgraphicsitem_cast<QGraphicsRectItem *>(item)) {
      return qgraphicsitem_cast<QGraphicsRectItem *>(item);
    }
  }
  return nullptr;
}

ImageTraceGraphicsView::QGraphicsContourPathsItem* ImageTraceGraphicsView::getTraceContourPathsItem() {
  for (auto item: scene()->items()) {
    if (item->data(ITEM_ID_KEY).toString().compare(QString(IMAGE_TRACE_ITEM_ID)) == 0 &&
        qgraphicsitem_cast<QGraphicsContourPathsItem *>(item)) {
      return qgraphicsitem_cast<QGraphicsContourPathsItem *>(item);
    }
  }
  return nullptr;
}
