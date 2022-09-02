#pragma once

#include <QMouseEvent>
#include <canvas/controls/canvas-control.h>

namespace Controls {

  /**
   \class Line
   \brief The Line class represents the drawing control for lines
   */
  class Line : public CanvasControl {
  public:
    explicit Line(Canvas *canvas) : CanvasControl(canvas) {
      direction_locked_ = false;
    }

    bool mouseMoveEvent(QMouseEvent *e) override;

    bool mouseReleaseEvent(QMouseEvent *e) override;

    bool keyPressEvent(QKeyEvent *e) override;

    void paint(QPainter *painter) override;

    void exit() override;

    bool isActive() override;

    bool isDirectionLock() const;

    void setDirectionLock(bool direction_lock);

  private:
    QPointF cursor_;

    bool direction_locked_;
  };

}