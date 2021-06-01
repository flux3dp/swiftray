#include <QMouseEvent>
#include <canvas/controls/canvas_control.h>

#ifndef RECTDRAWER_H
#define RECTDRAWER_H

class RectDrawer : CanvasControl {
    public:
        RectDrawer(Scene &scene_) noexcept: CanvasControl(scene_) {}
        bool mousePressEvent(QMouseEvent *e) override;
        bool mouseMoveEvent(QMouseEvent *e) override;
        bool mouseReleaseEvent(QMouseEvent *e) override;
};

#endif // RECTDRAWER_H
