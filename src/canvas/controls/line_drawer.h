#include <QMouseEvent>
#include <canvas/controls/canvas_control.h>

#ifndef LINEDRAWER_H
#define LINEDRAWER_H

class LineDrawer : public CanvasControl {
  public:
    LineDrawer(Scene &scene_) noexcept : CanvasControl(scene_) {}
    bool mousePressEvent(QMouseEvent *e) override;
    bool mouseMoveEvent(QMouseEvent *e) override;
    bool mouseReleaseEvent(QMouseEvent *e) override;
    void paint(QPainter *painter) override;
    void reset();

  private:
    QPointF cursor_;
};

#endif // LINEDRAWER_H
