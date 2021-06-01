#ifndef RECTDRAWER_H
#define RECTDRAWER_H

#include <QMouseEvent>
#include <canvas/canvas_control.h>


class RectDrawer : CanvasControl {
    public:
        bool mousePressEvent(QMouseEvent *e) override;
        bool mouseMoveEvent(QMouseEvent *e) override;
        bool mouseReleaseEvent(QMouseEvent *e) override;
};

#endif // RECTDRAWER_H
