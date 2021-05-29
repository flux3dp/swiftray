#include <limits>
#include <cmath>
#include <QMouseEvent>
#include <QHoverEvent>
#include <canvas/canvas_data.hpp>
#include <shape/shape.hpp>

#ifndef TRANSFORM_BOX_H
#define TRANSFORM_BOX_H

class TransformBox : QObject {
        Q_OBJECT
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
        TransformBox(CanvasData &canvas_data) noexcept;
        const QPointF *controlPoints();
        QRectF boundingRect();
        bool mousePressEvent(QMouseEvent *e);
        bool mouseReleaseEvent(QMouseEvent *e);
        bool mouseMoveEvent(QMouseEvent *e);
        bool hoverEvent(QHoverEvent *e, Qt::CursorShape *cursor);
        void paint(QPainter *painter);
        QList<ShapePtr> &selections();
        CanvasData &canvas();
        void move(QPointF offset);

    private:
        void rotate(double rotation);
        void scale(QPointF scaleCenter, float scaleX, float scaleY);
        ControlPoint testHit(QPointF clickPoint, float tolerance);
        QPointF control_points_[8];
        ControlPoint activating_control_;
        QPointF pressed_at_;
        CanvasData &canvas_;
        QPointF action_center_;
        float cumulated_rotation_;
        bool flipped_x;
        bool flipped_y;
        QRectF init_rotation_rect_;
        QRectF bounding_rect_;
        QList<ShapePtr> selections_;

        float transform_rotation;
        QSizeF transform_scaler;
    public slots:
        void updateSelections();
        void updateBoundingRect();
    signals:
        void transformChanged();
};

#endif
