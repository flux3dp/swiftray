#include <QMouseEvent>
#include <canvas/controls/canvas-control.h>

#ifndef CONTROL_OVAL_H
#define CONTROL_OVAL_H
namespace Controls {

  class Oval : public CanvasControl {
  public:
    Oval(Document &scene_) noexcept: CanvasControl(scene_) {}

    bool mouseMoveEvent(QMouseEvent *e) override;

    bool mouseReleaseEvent(QMouseEvent *e) override;

    void paint(QPainter *painter) override;

    void reset();

    bool isActive() override;

  private:
    QRectF rect_;
  };

}
#endif // CONTROL_OVAL_H
