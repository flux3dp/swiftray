#pragma once

#include <QMouseEvent>
#include <canvas/controls/canvas-control.h>

namespace Controls {

  class Oval : public CanvasControl {
  public:
    explicit Oval(Canvas *canvas) : CanvasControl(canvas) {}

    bool mouseMoveEvent(QMouseEvent *e) override;

    bool mouseReleaseEvent(QMouseEvent *e) override;

    bool keyPressEvent(QKeyEvent *e) override;

    void paint(QPainter *painter) override;

    void exit() override;

    bool isActive() override;

  private:
    QRectF rect_;
  };

}