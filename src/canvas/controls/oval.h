#include <QMouseEvent>
#include <canvas/controls/canvas-control.h>

#ifndef CONTROL_OVAL_H
#define CONTROL_OVAL_H
namespace Controls {

  class Oval : public CanvasControl {
  public:
    Oval(Canvas *canvas) noexcept: CanvasControl(canvas) {}

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
#endif // CONTROL_OVAL_H
