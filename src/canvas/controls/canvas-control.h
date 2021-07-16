#pragma once

#include <QMouseEvent>
#include <QObject>
#include <QPainter>
#include <document.h>

class Canvas;


/**
  \namespace Controls
  \brief In-canvas controls with its own painting and event handling functions.
*/
namespace Controls {
  class CanvasControl : public QObject {
  Q_OBJECT
  public:
    explicit CanvasControl(Canvas *parent);

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
}