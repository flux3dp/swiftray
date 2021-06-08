#include <QMouseEvent>
#include <QObject>
#include <QPainter>
#include <canvas/scene.h>

#ifndef CANVAS_CONTROL_H
#define CANVAS_CONTROL_H

namespace Controls {
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
    virtual bool isActive(); // Return if the control is active in the correct mode

    Scene &scene();

  private:
    Scene &scene_;
};

// Possible control classes
class Grid;
class Line;
class Oval;
class PathDraw;
class PathEdit;
class Rect;
class Select;
class Text;
class Transform;

}
#endif // CANVAS_CONTROL_H
