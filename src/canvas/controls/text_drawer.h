#include <QMouseEvent>
#include <canvas/controls/canvas_control.h>
#include <shape/text_shape.h>

#ifndef TEXTDRAWER_H
#define TEXTDRAWER_H

class TextDrawer : public CanvasControl {
    public:
        TextDrawer(Scene &scene_) noexcept: CanvasControl(scene_) {}
        bool mousePressEvent(QMouseEvent *e) override;
        bool mouseMoveEvent(QMouseEvent *e) override;
        bool mouseReleaseEvent(QMouseEvent *e) override;
        bool hoverEvent(QHoverEvent *e, Qt::CursorShape *cursor) override;
        bool keyPressEvent(QKeyEvent *e) override;
        void paint(QPainter *painter) override;
        void reset();
        TextShape &target();
        void setTarget(ShapePtr target);
    private:
        QPointF origin_;
        ShapePtr target_;
        int blink_counter;
};

#endif // TEXTDRAWER_H
