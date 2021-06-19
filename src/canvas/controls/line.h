#include <QMouseEvent>
#include <canvas/controls/canvas-control.h>

#ifndef CONTROL_LINE_H
#define CONTROL_LINE_H
namespace Controls {

  class Line : public CanvasControl {
  public:
    Line(Canvas *canvas) noexcept: CanvasControl(canvas) {}

    bool mouseMoveEvent(QMouseEvent *e) override;

    bool mouseReleaseEvent(QMouseEvent *e) override;

    bool keyPressEvent(QKeyEvent *e) override;

    void paint(QPainter *painter) override;

    void exit() override;

    bool isActive() override;

  private:
    QPointF cursor_;
  };

}

#endif // CONTROL_LINE_H
