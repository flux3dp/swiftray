#include <canvas/controls/canvas-control.h>

using namespace Controls;

CanvasControl::CanvasControl(Document &scene)
     : scene_(scene) {

};

Document &CanvasControl::scene() { return scene_; }

bool CanvasControl::isActive() { return false; }

// Event Handlers - return true if the event is handled in this control when isActive==true
bool CanvasControl::mousePressEvent(QMouseEvent *e) { return true; }

bool CanvasControl::mouseMoveEvent(QMouseEvent *e) { return true; }

bool CanvasControl::mouseReleaseEvent(QMouseEvent *e) { return true; }

bool CanvasControl::hoverEvent(QHoverEvent *e, Qt::CursorShape *cursor) {
  return false;
}

bool CanvasControl::keyPressEvent(QKeyEvent *e) { return true; }

// Paint
void CanvasControl::paint(QPainter *painter) {}