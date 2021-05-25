#include <limits>
#include <cmath>
#include <shape/shape.hpp>

#ifndef TRANSFORM_BOX_H
#define TRANSFORM_BOX_H

class TransformBox {
    public:
        TransformBox() noexcept;
        void setTarget(Shape *target);
        void setTargets(QList<Shape *> &targets);
        void clear();
        void rotate(double rotation);
        void move(QPointF offset);
        const QPointF *controlPoints();
        QRectF boundingRect();
    private:
        QList<Shape *> *targets();
        QPointF rotatePointAround(QPointF p, QPointF center, double theta);
        QList<Shape *> targets_;
        QPointF control_points_[8];
};

#endif
