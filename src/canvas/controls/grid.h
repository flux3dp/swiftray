#pragma once

#include <QMouseEvent>
#include <canvas/controls/canvas-control.h>

namespace Controls {

  class Grid : public CanvasControl {
  public:
    explicit Grid(Canvas *canvas) : CanvasControl(canvas) {}

    void paint(QPainter *painter) override;

    bool isActive() override;
  };

}