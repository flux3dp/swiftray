#include <QMouseEvent>
#include <canvas/controls/canvas-control.h>

#ifndef RECT_H
#define RECT_H
namespace Controls {

  class Rect : public CanvasControl {
  public:
    Rect(Canvas *canvas) noexcept: CanvasControl(canvas) {}

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
#endif // RECT_H
