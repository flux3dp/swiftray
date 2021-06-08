#include <QMouseEvent>
#include <canvas/controls/canvas_control.h>

#ifndef GRID_H
#define GRID_H

class Grid : public CanvasControl {
  public:
    Grid(Scene &scene_) noexcept : CanvasControl(scene_) {}
    void paint(QPainter *painter) override;
};

#endif // GRID_H
