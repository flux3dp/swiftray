#include <QPainter>
#include <QPainterPath>

#ifndef SHAPE_H
#define SHAPE_H
// Use like struct first (can run faster..)
class Shape {
    public:
        QPainterPath path;
        bool selected;
        double x;
        double y;
        void simplify();
        bool testHit(QPointF point);
        QList<QPointF> polyCache;
};
#endif //SHAPE_H
