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
        double scale_x;
        double scale_y;
        void simplify();
        QPointF pos();
        QRectF boundingRect();
        bool testHit(QPointF point);
    private:
        void cacheSelectionTestingData();
        QList<QPointF> selection_testing_points;
        QRectF selection_testing_rect;
};
#endif //SHAPE_H
