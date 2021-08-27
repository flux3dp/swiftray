#pragma once

#include <widgets/components/base-graphicsview.h>
#include <QGraphicsView>
#include <QGestureEvent>

class ImageTraceGraphicsView: public BaseGraphicsView {
  Q_OBJECT

public:

    ImageTraceGraphicsView(QWidget *parent = nullptr);
    ImageTraceGraphicsView(QGraphicsScene *scene = nullptr, QWidget *parent = nullptr);
    void reset();

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    void updateBackgroundPixmap(QPixmap background_img);
    void updateTrace(QPainterPath contours);
    void clearSelectionArea();
    void drawSelectionArea();

signals:
    void selectionAreaChanged();

private:
    // Or use enum
    constexpr static int BACKGROUND_IMAGE_Z_INDEX = 0;
    constexpr static int SELECTION_RECT_Z_INDEX = 1;
    constexpr static int IMAGE_TRACE_Z_INDEX = 2;

    QGraphicsPixmapItem* bg_image_item_ = nullptr;
    QGraphicsRectItem* selection_area_rect_item_ = nullptr;
    QGraphicsPathItem* contours_path_item_ = nullptr;
};