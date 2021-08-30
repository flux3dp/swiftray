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
    void updateTrace(const QPainterPath& contours);
    void updateAnchorPoints(const QPainterPath& contours, bool show_points);
    void clearSelectionArea();
    void drawSelectionArea();

signals:
    void selectionAreaChanged();

private:
    constexpr static int ITEM_ID_KEY = 0;

    constexpr static int BACKGROUND_IMAGE_Z_INDEX = 0;
    constexpr static char BACKGROUND_IMAGE_ITEM_ID[] = "BACKGROUND";
    constexpr static int SELECTION_RECT_Z_INDEX = 1;
    constexpr static char SELECTION_RECT_ITEM_ID[] = "SELECT_RECT";
    constexpr static int IMAGE_TRACE_Z_INDEX = 2;
    constexpr static char IMAGE_TRACE_ITEM_ID[] = "TRACE";
    constexpr static int ANCHOR_POINTS_Z_INDEX = 3;
    constexpr static char ANCHOR_POINTS_ITEM_ID[] = "POINTS";

    // Might replace the following with a template function
    QGraphicsPixmapItem* getBackgroundPixmapItem();
    QGraphicsRectItem* getSelectionAreaRectItem();
    QGraphicsPathItem* getTraceContourPathsItem();
    QGraphicsEllipseItem* getTraceContourAnchorItem();
};