#include <QPainter>
#include <QPainterPath>

#ifndef SHAPE_H
#define SHAPE_H
class Shape {
        QPainterPath path;
        bool selected;
        double x;
        double y;
        void simplify();
};
#endif //SHAPE_H
