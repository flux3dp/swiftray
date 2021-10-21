#pragma once

#include <QMouseEvent>
#include <canvas/controls/canvas-control.h>

namespace Controls {

  class Rect : public CanvasControl {
  public:
    Rect(Canvas *canvas) noexcept: CanvasControl(canvas) {}

    bool mouseMoveEvent(QMouseEvent *e) override;

    bool mouseReleaseEvent(QMouseEvent *e) override;

    bool keyPressEvent(QKeyEvent *e) override;

    bool keyReleaseEvent(QKeyEvent *e) override;

    void paint(QPainter *painter) override;

    void exit() override;

    bool isActive() override;

    void setScaleLock(bool scale_lock);

  private:
    QRectF rect_;
    QPointF mouse_pos_;
    QPointF pressed_pos_;
    bool scale_locked_;
    float aspect_ratio_ = 1;
  };
}
