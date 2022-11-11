#pragma once

#include <QHoverEvent>
#include <QMouseEvent>
#include <canvas/controls/canvas-control.h>
#include <document.h>
#include <cmath>
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

    const QPointF *controlPoints();

    QRectF boundingRect();

    QList<ShapePtr> &selections();

    void calcScale(QPointF canvas_coord);

    void applyMove(bool temporarily = false);

    double rotation() {
      return bbox_angle_;
    }

    void updateTransform(double new_x, double new_y, double new_r, double new_w, double new_h) {
      if (std::abs(new_x - x()) > 0.01 || std::abs(new_y - y()) > 0.01) {
        translate_to_apply_ = QPointF(new_x - x(), new_y - y());
        applyMove();
      }
      if (std::abs(new_r - rotation()) > 0.01) {
        rotation_to_apply_ = new_r - rotation();
        applyRotate(boundingRect().center(), rotation_to_apply_);
      }
      if (std::abs(new_w - width()) > 0.01 || std::abs(new_h - height()) > 0.01) {
        scale_x_to_apply_ = width() == 0 ? 1 : new_w / width();
        scale_y_to_apply_ = height() == 0 ? 1 : new_h / height();
        applyScale(boundingRect().center(), scale_x_to_apply_, scale_y_to_apply_);
      }
    }

    double x() {
      return boundingRect().center().x();
    }

    double y() {
      return boundingRect().center().y();
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

  private:
    void applyRotate(QPointF center, double rotation, bool temporarily = false);

    Control hitTest(QPointF clickPoint, float tolerance);

    QPointF controls_[9];
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

    bool scale_locked_;
    bool direction_locked_;

  public Q_SLOTS:
    void updateSelections();

    void updateBoundingRect();

  Q_SIGNALS:
    void cursorChanged(Qt::CursorShape cursor);
  };
}
