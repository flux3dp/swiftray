#include <QHoverEvent>
#include <QMouseEvent>
#include <canvas/controls/canvas_control.h>
#include <canvas/scene.h>
#include <cmath>
#include <limits>
#include <shape/shape.h>

#ifndef TRANSFORM_H
#define TRANSFORM_H

namespace Controls {

class Transform : public CanvasControl {
    Q_OBJECT
  public:
    enum class Control {
        NONE = -1,
        NW = 0,
        N = 1,
        NE = 2,
        E = 3,
        SE = 4,
        S = 5,
        SW = 6,
        W = 7,
        ROTATION = 8
    };
    Transform(Scene &scene) noexcept;
    bool keyPressEvent(QKeyEvent *e) override;
    bool hoverEvent(QHoverEvent *e, Qt::CursorShape *cursor) override;
    bool mousePressEvent(QMouseEvent *e) override;
    bool mouseReleaseEvent(QMouseEvent *e) override;
    bool mouseMoveEvent(QMouseEvent *e) override;
    void paint(QPainter *painter) override;
    bool isActive() override;
    void reset();
    const QPointF *controlPoints();
    QRectF boundingRect();
    QList<ShapePtr> &selections();
    void calcScale(QPointF canvas_coord);
    void applyMove(bool temporarily = false);

  private:
    void applyRotate(bool temporarily = false);
    void applyScale(bool temporarily = false);
    Control hitTest(QPointF clickPoint, float tolerance);
    QPointF controls_[8];
    Control active_control_;
    QPointF action_center_;
    QRectF bounding_rect_;

    QList<ShapePtr> selections_;

    double scale_x_to_apply_;
    double scale_y_to_apply_;
    double rotation_to_apply_;
    QPointF translate_to_apply_;

    QPointF cursor_;

    double rotated_from_;
    QSizeF transformed_from_;

    qreal bbox_angle_;
    bool bbox_need_recalc_;
  public slots:
    void updateSelections();
    void updateBoundingRect();
  signals:
    void transformChanged();
};

}
#endif