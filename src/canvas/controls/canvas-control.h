#include <QMouseEvent>
#include <QObject>
#include <QPainter>
#include <document.h>

#ifndef CANVAS_CONTROL_H
#define CANVAS_CONTROL_H

class Canvas;

namespace Controls {
  class CanvasControl : public QObject {
  Q_OBJECT
  public:
    CanvasControl(Canvas *parent);

    virtual bool mousePressEvent(QMouseEvent *e);

    virtual bool mouseMoveEvent(QMouseEvent *e);

    virtual bool mouseReleaseEvent(QMouseEvent *e);

    virtual bool hoverEvent(QHoverEvent *e, Qt::CursorShape *cursor);

    virtual bool keyPressEvent(QKeyEvent *e);

    virtual void paint(QPainter *painter);

    // Return if the control is active in the correct mode
    virtual bool isActive();

    virtual void exit();

    Canvas &canvas();

    Document &document();

  private:
    Canvas *canvas_;
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
