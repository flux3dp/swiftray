#pragma once

#include <widgets/components/base-graphicsview.h>
#include <QGraphicsView>
#include <QGestureEvent>
#include <QGraphicsItem>
#include <widgets/components/graphicitems/resizeable-rect-item.h>

class ImageCropGraphicsView: public BaseGraphicsView {
Q_OBJECT

public:
    ImageCropGraphicsView(QWidget *parent = nullptr);

    void updateTargetImage(const QImage &image);
    void updateTargetImage(QImage &&image);
    void updateBackgroundPixmap(QPixmap background_img) override;

    QImage getCropImage();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void pinchGestureHandler(QPinchGesture *pg) override;

private:
    constexpr static int ITEM_ID_KEY = 0;

    constexpr static int BACKGROUND_IMAGE_Z_INDEX = 0;
    constexpr static char BACKGROUND_IMAGE_ITEM_ID[] = "BACKGROUND";
    constexpr static int CROP_AREA_Z_INDEX = 1;
    constexpr static char CROP_AREA_ITEM_ID[] = "CROP AREA";

    QPoint new_crop_area_starting_point_;
    ResizeableRectItem* getCropAreaItem();
    QImage target_image_; // temporary image as crop target
    ResizeableRectItem *crop_area_ = nullptr;
    bool creating_new_crop_area_ = false;
};
