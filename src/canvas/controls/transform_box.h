#include <limits>
#include <cmath>
#include <QMouseEvent>
#include <QHoverEvent>
#include <canvas/scene.h>
#include <canvas/controls/canvas_control.h>
#include <shape/shape.h>

#ifndef TRANSFORM_BOX_H
#define TRANSFORM_BOX_H

class TransformBox : public CanvasControl {
        Q_OBJECT
    public:
        enum class ControlPoint {
            NONE = -1,
            NW = 0,
            N = 1,
            NE = 2,
            E = 3,
            SE = 4,
            S = 5,
            SW= 6,
            W = 7,
            ROTATION = 8
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
        void calcScale(QPointF canvas_coord);
        void applyMove(bool temporarily = false);

    private:
        void applyRotate(bool temporarily = false);
        void applyScale(bool temporarily = false);
        ControlPoint hitTest(QPointF clickPoint, float tolerance);
        QPointF control_points_[8];
        ControlPoint activating_control_;
        QPointF action_center_;
        bool flipped_x;
        bool flipped_y;
        QRectF init_rotation_rect_;
        QRectF bounding_rect_;
        QList<ShapePtr> selections_;

        double scale_x_to_apply_;
        double scale_y_to_apply_;
        double rotation_to_apply_;
        QPointF translate_to_apply_;

        QPointF cursor_;

        double rotated_from;
        QSizeF transformed_from_;

        qreal bbox_rotation_;
        bool bbox_need_recalc_;
    public slots:
        void updateSelections();
        void updateBoundingRect();
    signals:
        void transformChanged();
};

#endif
