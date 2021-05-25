#include <QPainter>
#include <QPainterPath>

#ifndef SHAPE_H
#define SHAPE_H
// Use like struct first (can run faster..)
class Shape {
    public:
        Shape() noexcept;
        QPainterPath path;
        bool selected;
        double x;
        double y;
        double rot;
        double scaleX;
        double scaleY;
        void simplify();
        void setCenter(QPointF newCenterPos);
        QPointF pos();
        QRectF boundingRect();
        bool testHit(QPointF point);
    private:
        void cacheSelectionTestingData();
        QRectF transformedBBox;
        QPainterPath transformedPath;
        QList<QPointF> selectionTestingPoints;
        QRectF selectionTestingRect;
};
#endif //SHAPE_H
