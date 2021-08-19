#pragma once

#include <QMouseEvent>
#include <canvas/controls/canvas-control.h>

namespace Controls {

  class Line : public CanvasControl {
  public:
    explicit Line(Canvas *canvas) : CanvasControl(canvas) {}

    bool mouseMoveEvent(QMouseEvent *e) override;

    bool mouseReleaseEvent(QMouseEvent *e) override;

    bool keyPressEvent(QKeyEvent *e) override;

    void paint(QPainter *painter) override;

    void exit() override;

    bool isActive() override;

  private:
    QPointF cursor_;
  };

}