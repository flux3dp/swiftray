#ifndef CANVAS_CONTROL_H
#define CANVAS_CONTROL_H

#include <QObject>
#include <QPainter>
#include <QMouseEvent>
#include <canvas/scene.h>

class CanvasControl : public QObject {
        Q_OBJECT
    public:
        CanvasControl(Scene &scene) : scene_ { scene } {};
        virtual bool mousePressEvent(QMouseEvent *e);
        virtual bool mouseMoveEvent(QMouseEvent *e);
        virtual bool mouseReleaseEvent(QMouseEvent *e);
        virtual bool hoverEvent(QHoverEvent *e, Qt::CursorShape *cursor);
        virtual void paint(QPainter *painter);

        Scene &scene() {
            return scene_;
        }
        Scene &scene_;
};

#endif // CANVAS_CONTROL_H
