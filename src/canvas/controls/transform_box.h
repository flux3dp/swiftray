#include <limits>
#include <cmath>
#include <QMouseEvent>
#include <QHoverEvent>
#include <canvas/scene.h>
#include <canvas/controls/canvas_control.h>
#include <shape/shape.h>

#ifndef TRANSFORM_BOX_H
#define TRANSFORM_BOX_H

class TransformBox : CanvasControl {
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
        TransformBox(Scene &scene) noexcept;
        bool mousePressEvent(QMouseEvent *e) override;
        bool mouseReleaseEvent(QMouseEvent *e) override;
        bool mouseMoveEvent(QMouseEvent *e) override;
        bool hoverEvent(QHoverEvent *e, Qt::CursorShape *cursor) override;
        void paint(QPainter *painter) override;
        const QPointF *controlPoints();
        QRectF boundingRect();
        QList<ShapePtr> &selections();
        void move(QPointF offset);

    private:
        void rotate(double rotation);
        void scale(QPointF scaleCenter, float scaleX, float scaleY);
        ControlPoint testHit(QPointF clickPoint, float tolerance);
        QPointF control_points_[8];
        ControlPoint activating_control_;
        QPointF pressed_at_;
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
