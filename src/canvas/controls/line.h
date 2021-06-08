#include <QMouseEvent>
#include <canvas/controls/canvas_control.h>

#ifndef CONTROL_LINE_H
#define CONTROL_LINE_H
namespace Controls {

class Line : public CanvasControl {
  public:
    Line(Scene &scene_) noexcept : CanvasControl(scene_) {}
    bool mouseMoveEvent(QMouseEvent *e) override;
    bool mouseReleaseEvent(QMouseEvent *e) override;
    void paint(QPainter *painter) override;
    void reset();
    bool isActive() override;

  private:
    QPointF cursor_;
};

}

#endif // CONTROL_LINE_H
