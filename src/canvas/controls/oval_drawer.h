#include <QMouseEvent>
#include <canvas/controls/canvas_control.h>

#ifndef OVALDRAWER_H
#define OVALDRAWER_H

class OvalDrawer : public CanvasControl {
  public:
    OvalDrawer(Scene &scene_) noexcept : CanvasControl(scene_) {}
    bool mouseMoveEvent(QMouseEvent *e) override;
    bool mouseReleaseEvent(QMouseEvent *e) override;
    void paint(QPainter *painter) override;
    void reset();
    bool isActive() override;

  private:
    QRectF rect_;
};

#endif // OVALDRAWER_H
