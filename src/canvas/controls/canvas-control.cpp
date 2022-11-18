#include <canvas/controls/canvas-control.h>
#include <canvas/canvas.h>

using namespace Controls;

CanvasControl::CanvasControl(Canvas *canvas) :
     canvas_(canvas) {}

/* Returns true if this control is active in current canvas state */
bool CanvasControl::isActive() { return false; }

/* Returns true if the event is handled in this control when isActive==true */
bool CanvasControl::mousePressEvent(QMouseEvent *e) { return true; }

/* Returns true if the event is handled in this control when isActive==true */
bool CanvasControl::mouseMoveEvent(QMouseEvent *e) { return true; }

/* Returns true if the event is handled in this control when isActive==true */
bool CanvasControl::mouseReleaseEvent(QMouseEvent *e) { return true; }

/* Returns true if the event is handled in this control when isActive==true */
bool CanvasControl::hoverEvent(QHoverEvent *e, Qt::CursorShape *cursor) { return false; }

/* Return true if the event is handled in this control when isActive==true */
bool CanvasControl::keyPressEvent(QKeyEvent *e) { return true; }

/* Return true if the event is handled in this control when isActive==true */
bool CanvasControl::keyReleaseEvent(QKeyEvent *e) { return true; }

void CanvasControl::paint(QPainter *painter) {}

/* Exit will be called when we're exiting the active state */
void CanvasControl::exit() {
  // TODO (Reimplement exit as a slot being called by Q_SIGNALS)
}

Canvas &CanvasControl::canvas() {
  return *canvas_;
}

Document &CanvasControl::document() {
  return canvas_->document();
}