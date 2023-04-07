#pragma once

#include <QHoverEvent>
#include <QMouseEvent>
#include <canvas/controls/canvas-control.h>
#include <document.h>
#include <cmath>
#include <constants.h>
#include <limits>
#include <shape/shape.h>

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

    Transform(Canvas *canvas) noexcept;

    bool keyPressEvent(QKeyEvent *e) override;

    bool hoverEvent(QHoverEvent *e, Qt::CursorShape *cursor) override;

    bool mousePressEvent(QMouseEvent *e) override;

    bool mouseReleaseEvent(QMouseEvent *e) override;

    bool mouseMoveEvent(QMouseEvent *e) override;

    void paint(QPainter *painter) override;

    bool isActive() override;

    void reset();

    void controlPoints();

    QRectF boundingRect();

    QList<ShapePtr> &selections();

    void calcScale(QPointF canvas_coord);

    void applyMove(bool temporarily = false);

    double rotation() {
      return bbox_angle_;
    }

    void updateTransform(double new_x, double new_y, double new_r, double new_w, double new_h);

    double x() {
      updateReferencePoint();
      return reference_point_.x();
    }

    double y() {
      updateReferencePoint();
      return reference_point_.y();
    }

    double width() {
      return boundingRect().width();
    }

    double height() {
      return boundingRect().height();
    }

    bool isScaleLock() const;

    void setScaleLock(bool scale_lock);

    bool isDirectionLock() const;

    void setDirectionLock(bool direction_lock);

    void applyScale(QPointF center, double scale_x, double scale_y, bool temporarily = false);

    void setReferencePoint(JobOrigin reference_point);

  private:
    void applyRotate(QPointF center, double rotation, bool temporarily = false);

    Control hitTest(QPointF clickPoint, float tolerance);

    void updateReferencePoint();

    QPointF controls_[9];
    Control active_control_;
    QPointF action_center_;
    JobOrigin reference_origin_;
    QPointF reference_point_;
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

    bool scale_locked_;
    bool direction_locked_;

  public Q_SLOTS:
    void updateSelections(QList<ShapePtr> selections);
    void updateBoundingRect();
  };
}
