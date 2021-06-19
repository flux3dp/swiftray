#include <QMouseEvent>
#include <canvas/controls/canvas-control.h>

#ifndef CONTROL_GRID_H
#define CONTROL_GRID_H
namespace Controls {

  class Grid : public CanvasControl {
  public:
    Grid(Canvas *canvas) noexcept: CanvasControl(canvas) {}

    void paint(QPainter *painter) override;

    bool isActive() override;
  };

} // namespace Controls
#endif // CONTROL_GRID_H
