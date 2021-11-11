#include "image-crop-graphicsview.h"

#include <QDebug>
#include <widgets/components/graphicitems/resizeable-rect-item.h>

ImageCropGraphicsView::ImageCropGraphicsView(QWidget *parent)
        : BaseGraphicsView(parent)
{
  setMinScale(0.2);
  //setDragMode(QGraphicsView::NoDrag);
  setDragMode(QGraphicsView::RubberBandDrag);

  setViewportMargins(0, 16, 0, -2);
  setFrameStyle(QFrame::NoFrame);

  setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}

void ImageCropGraphicsView::mousePressEvent(QMouseEvent *event) {
  QGraphicsView::mousePressEvent(event);
  // Handle click outside -> create a new crop area
  if ( crop_area_ &&
      !crop_area_->contains(mapToScene(event->pos())) ) {
    creating_new_crop_area_ = true;
    crop_area_->setVisible(false); // hided temporarily
  }
}

void ImageCropGraphicsView::mouseReleaseEvent(QMouseEvent *event) {
  if (creating_new_crop_area_) {
    if ( crop_area_ ) {
      crop_area_->updateRect(
              QRectF(
                      mapToScene(rubberBandRect().topLeft()),
                      mapToScene(rubberBandRect().bottomRight())
                      )
                    );
      crop_area_->setVisible(true);
    }
    creating_new_crop_area_ = false;
  }
  QGraphicsView::mouseReleaseEvent(event);
}

/**
 * @brief clear and draw new background image
 * @param background_pixmap
 */
void ImageCropGraphicsView::updateBackgroundPixmap(QPixmap background_pixmap) {
  auto item = getBackgroundPixmapItem();
  if (item) {
    item->setPixmap(background_pixmap);
  } else {
    // If not exist -> create and add a new graphics item
    item = scene()->addPixmap(background_pixmap);
    item->setData(ITEM_ID_KEY, BACKGROUND_IMAGE_ITEM_ID);
    item->setZValue(BACKGROUND_IMAGE_Z_INDEX); // overlapped by any other items
  }

  // Set crop area to full image
  if (crop_area_) {
    crop_area_->updateRect(QRectF(0, 0, background_pixmap.width(), background_pixmap.height()));
    crop_area_->setMaxBoundary(QRectF(0, 0, background_pixmap.width(), background_pixmap.height()));
    setSceneRect(crop_area_->boundingRect());
    resetTransform();
    fitInView(crop_area_->boundingRect(), Qt::KeepAspectRatio);
  } else {
    crop_area_ = new ResizeableRectItem(0, 0, background_pixmap.width(), background_pixmap.height());
    crop_area_->setZValue(CROP_AREA_Z_INDEX);
    crop_area_->setData(ITEM_ID_KEY, CROP_AREA_ITEM_ID);
    crop_area_->setMaxBoundary(QRectF(0, 0, background_pixmap.width(), background_pixmap.height()));
    this->scene()->addItem(crop_area_);
    //setSceneRect(-10, -10, crop_area_->boundingRect().width(), crop_area_->boundingRect().height());
    setSceneRect(crop_area_->boundingRect());
    resetTransform();
    fitInView(crop_area_->boundingRect(), Qt::KeepAspectRatio);
  }
}

QGraphicsPixmapItem* ImageCropGraphicsView::getBackgroundPixmapItem() {
  for (auto item: scene()->items()) {
    if (item->data(ITEM_ID_KEY).toString().compare(QString(BACKGROUND_IMAGE_ITEM_ID)) == 0 &&
        qgraphicsitem_cast<QGraphicsPixmapItem *>(item)) {
      return qgraphicsitem_cast<QGraphicsPixmapItem *>(item);
    }
  }
  return nullptr;
}

/**
 * @brief
 *        NOTE: Use this API or store crop_area_ directly
 * @return
 */
ResizeableRectItem* ImageCropGraphicsView::getCropAreaItem() {
  for (auto item: scene()->items()) {
    if (item->data(ITEM_ID_KEY).toString().compare(QString(CROP_AREA_ITEM_ID)) == 0 &&
        qgraphicsitem_cast<ResizeableRectItem *>(item)) {
      return qgraphicsitem_cast<ResizeableRectItem *>(item);
    }
  }
  return nullptr;
}

QPixmap ImageCropGraphicsView::getCrop() {
  if (crop_area_ != nullptr && getBackgroundPixmapItem() != nullptr) {
    return getBackgroundPixmapItem()->pixmap().copy(
            crop_area_->rect().toRect()
    );
  }
  return QPixmap();
}