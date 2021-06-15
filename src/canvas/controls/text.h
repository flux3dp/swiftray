#include <QMouseEvent>
#include <canvas/controls/canvas-control.h>
#include <shape/text-shape.h>

#ifndef TEXTDRAWER_H
#define TEXTDRAWER_H
namespace Controls {

  class Text : public CanvasControl {
  public:
    Text(Document &scene_) noexcept: CanvasControl(scene_) {}

    bool mouseReleaseEvent(QMouseEvent *e) override;

    bool hoverEvent(QHoverEvent *e, Qt::CursorShape *cursor) override;

    bool keyPressEvent(QKeyEvent *e) override;

    void paint(QPainter *painter) override;

    bool isActive() override;

    void reset();

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
