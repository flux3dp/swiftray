#include <QMouseEvent>
#include <canvas/controls/canvas-control.h>

#ifndef CONTROL_GRID_H
#define CONTROL_GRID_H
namespace Controls {

  class Grid : public CanvasControl {
  public:
    Grid(Document &scene_) noexcept: CanvasControl(scene_) {}

    void paint(QPainter *painter) override;

    bool isActive() override;
  };

} // namespace Controls
#endif // CONTROL_GRID_H
