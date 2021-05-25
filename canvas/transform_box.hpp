#include <limits>
#include <cmath>
#include <shape/shape.hpp>

#ifndef TRANSFORM_BOX_H
#define TRANSFORM_BOX_H

class TransformBox {
    public:
        QList<Shape *> targets;
        TransformBox() noexcept;
        void setTarget(Shape *target);
        void setTargets(QList<Shape *> &targets);
        void clear();
        void rotate(double rotation);
        void move(QPointF offset);

        QRectF boundingRect();
    private:
        QPointF rotatePointAround(QPointF p, QPointF center, double theta);
};

#endif
