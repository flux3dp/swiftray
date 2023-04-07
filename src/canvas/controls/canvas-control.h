#pragma once

#include <QMouseEvent>
#include <QObject>
#include <QPainter>
#include <document.h>

class Canvas;

namespace Controls {
  /**
   \class CanvasControl
   \brief The CanvasControl class represents a interactive / display component inside a canvas.
   */
  class CanvasControl : public QObject {
  Q_OBJECT
  public:
    explicit CanvasControl(Canvas *parent);

    virtual bool mousePressEvent(QMouseEvent *e);

    virtual bool mouseMoveEvent(QMouseEvent *e);

    virtual bool mouseReleaseEvent(QMouseEvent *e);

    virtual bool hoverEvent(QHoverEvent *e, Qt::CursorShape *cursor);

    virtual bool keyPressEvent(QKeyEvent *e);

    virtual bool keyReleaseEvent(QKeyEvent *e);

    virtual void paint(QPainter *painter);

    /**
    * Return if the control is active in current canvas state
    */
    virtual bool isActive();

    virtual void exit();

    Canvas &canvas();

    Document &document();

  private:
    Canvas *canvas_;
    
  Q_SIGNALS:
    void canvasUpdated();
    void shapeUpdated();
  };
}