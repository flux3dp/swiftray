#pragma once

#include <QMouseEvent>
#include <canvas/controls/canvas-control.h>

namespace Controls {

  /**
   \class Oval
   \brief The Oval class represents the drawing control for ovals
   */
  class Oval : public CanvasControl {
  public:
    explicit Oval(Canvas *canvas) : CanvasControl(canvas) {
      scale_locked_ = false;
    }

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
