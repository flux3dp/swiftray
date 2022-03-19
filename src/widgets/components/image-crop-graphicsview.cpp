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
  QPointF mouse_pos_in_crop_coord = transform().map(mapToScene(event->pos()));
  // Handle click outside -> create a new crop area
  if ( crop_area_ &&
      !crop_area_->contains( mouse_pos_in_crop_coord) ) {
    creating_new_crop_area_ = true;
    crop_area_->setVisible(false); // hidden temporarily
  }
}

void ImageCropGraphicsView::mouseReleaseEvent(QMouseEvent *event) {
  if (creating_new_crop_area_) {
    if ( crop_area_ ) {
      // NOTE: The scale of crop_area should not apply the transform (ignore transform flag is set inside)
      //       But we still need to translate to the transformed scene origin
      crop_area_->updateRect(
              QRectF(
                      rubberBandRect().topLeft() - mapFromScene(QPoint(0, 0)),
                      rubberBandRect().bottomRight() - mapFromScene(QPoint(0, 0))
              )
      );
      crop_area_->setVisible(true);
    }
    creating_new_crop_area_ = false;
  }
  QGraphicsView::mouseReleaseEvent(event);
}

void ImageCropGraphicsView::pinchGestureHandler(QPinchGesture *pg) {
  // Override base, disable zoom in/out
}

void ImageCropGraphicsView::updateTargetImage(const QImage &image) {
  target_image_ = image;
  updateBackgroundPixmap(QPixmap::fromImage(target_image_));
}

void ImageCropGraphicsView::updateTargetImage(QImage &&image) {
  target_image_ = image;
  updateBackgroundPixmap(QPixmap::fromImage(target_image_));
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

  QRectF scene_rect{0, 0, qreal(background_pixmap.width()), qreal(background_pixmap.height())};
  setSceneRect(scene_rect);
  fitInView(sceneRect(), Qt::KeepAspectRatio);
  // Set crop area to full image
  // NOTE: Since ResizeableRectItem has ItemIgnoresTransformations flag set, 
  //       We need to manually perform the transform
  if (crop_area_) {
    crop_area_->updateRect(transform().mapRect(scene_rect));
  } else {
    crop_area_ = new ResizeableRectItem(transform().mapRect(scene_rect));
    crop_area_->setZValue(CROP_AREA_Z_INDEX);
    crop_area_->setData(ITEM_ID_KEY, CROP_AREA_ITEM_ID);
    this->scene()->addItem(crop_area_);
  }
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

QImage ImageCropGraphicsView::getCropImage() {
  if (crop_area_ != nullptr && getBackgroundPixmapItem() != nullptr) {
    // The PixmapItem is on scene and expressed in scene coordinate.
    // Thus, we need to apply "invert" the transform on crop_area_ to get the actual crop area on image (pixmap)
    return target_image_.copy(transform().inverted().mapRect(crop_area_->rect().toRect()));
  }
  return QImage();
}
