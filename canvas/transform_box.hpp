#include <limits>
#include <cmath>
#include <QMouseEvent>
#include <QHoverEvent>
#include <canvas/canvas_data.hpp>
#include <shape/shape.hpp>

#ifndef TRANSFORM_BOX_H
#define TRANSFORM_BOX_H

class TransformBox {
    public:
        enum class ControlPoint {
            NONE,
            NW,
            N,
            NE,
            E,
            SE,
            S,
            SW,
            W,
            ROTATION
        };
        TransformBox() noexcept;
        bool hasTargets();
        void setTarget(Shape *target);
        void setTargets(QList<Shape *> &targets);
        bool containsTarget(Shape *target);
        void clear();
        const QPointF *controlPoints();
        QRectF boundingRect();
        bool mousePressEvent(QMouseEvent *e, CanvasData &canvas_data);
        bool mouseReleaseEvent(QMouseEvent *e, const CanvasData &canvas_data);
        bool mouseMoveEvent(QMouseEvent *e, const CanvasData &canvas_data);
        bool hoverEvent(QHoverEvent *e, const CanvasData &canvas, Qt::CursorShape *cursor);
    private:
        void rotate(double rotation);
        void move(QPointF offset);
        void scale(QPointF scaleCenter, float scaleX, float scaleY);
        ControlPoint testHit(QPointF clickPoint, float tolerance);
        QList<Shape *> *targets();
        QList<Shape *> targets_;
        QPointF control_points_[8];
        ControlPoint activating_control_;
        QPointF pressed_at_;

        float transform_rotation;
        QSizeF transform_scaler;
};

#endif
