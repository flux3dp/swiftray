#include <QMouseEvent>
#include <QObject>
#include <QPainter>
#include <canvas/scene.h>

#ifndef CANVAS_CONTROL_H
#define CANVAS_CONTROL_H

class CanvasControl : public QObject {
    Q_OBJECT
  public:
    CanvasControl(Scene &scene);
    virtual bool mousePressEvent(QMouseEvent *e);
    virtual bool mouseMoveEvent(QMouseEvent *e);
    virtual bool mouseReleaseEvent(QMouseEvent *e);
    virtual bool hoverEvent(QHoverEvent *e, Qt::CursorShape *cursor);
    virtual bool keyPressEvent(QKeyEvent *e);
    virtual void paint(QPainter *painter);

    Scene &scene();
    QPointF dragged_from_screen_;
    QPointF dragged_from_canvas_;

  private:
    Scene &scene_;
};

#endif // CANVAS_CONTROL_H
