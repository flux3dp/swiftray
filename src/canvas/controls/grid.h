#pragma once

#include <QMouseEvent>
#include <canvas/controls/canvas-control.h>

namespace Controls {

  /**
   \class Grid
   \brief The Grid class represents the background grid of the canvas
   */
  class Grid : public CanvasControl {
  public:
    explicit Grid(Canvas *canvas) : CanvasControl(canvas) {}

    void paint(QPainter *painter) override;

    bool isActive() override;

    void setLineWidth(double new_width) {line_width_ = new_width;}

  private:
    //To set different width for different pixel ratio in canvas
    double line_width_ = 2;
  };

}