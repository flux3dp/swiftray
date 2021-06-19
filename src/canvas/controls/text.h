#include <QMouseEvent>
#include <canvas/controls/canvas-control.h>
#include <shape/text-shape.h>

#ifndef TEXTDRAWER_H
#define TEXTDRAWER_H
namespace Controls {

  class Text : public CanvasControl {
  public:
    Text(Canvas *canvas) noexcept: CanvasControl(canvas) {}

    bool mouseReleaseEvent(QMouseEvent *e) override;

    bool hoverEvent(QHoverEvent *e, Qt::CursorShape *cursor) override;

    bool keyPressEvent(QKeyEvent *e) override;

    void paint(QPainter *painter) override;

    bool isActive() override;

    void exit() override;

    TextShape &target();

    void setTarget(ShapePtr &target);

    bool hasTarget();

  private:
    QPointF origin_;
    ShapePtr target_;
    int blink_counter;
  };

}
#endif // TEXTDRAWER_H
