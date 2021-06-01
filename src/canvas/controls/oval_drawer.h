#include <QMouseEvent>
#include <canvas/controls/canvas_control.h>

#ifndef OVALDRAWER_H
#define OVALDRAWER_H

class OvalDrawer : CanvasControl {
    public:
        OvalDrawer(Scene &scene_) noexcept: CanvasControl(scene_) {}
        bool mousePressEvent(QMouseEvent *e) override;
        bool mouseMoveEvent(QMouseEvent *e) override;
        bool mouseReleaseEvent(QMouseEvent *e) override;
        void paint(QPainter *painter) override;
        void reset();
    private:
        QRectF rect_;
};

#endif // OVALDRAWER_H
