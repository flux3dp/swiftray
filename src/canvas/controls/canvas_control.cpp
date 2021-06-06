#include <canvas/controls/canvas_control.h>
CanvasControl::CanvasControl(Scene &scene) : scene_(scene) {

};

Scene &CanvasControl::scene() {
    return scene_;
}

bool CanvasControl::mousePressEvent(QMouseEvent *e) {
    dragged_from_screen_ = e->pos();
    dragged_from_canvas_ = scene().getCanvasCoord(e->pos());
    return false;
}

bool CanvasControl::mouseMoveEvent(QMouseEvent *e) { return false; }
bool CanvasControl::mouseReleaseEvent(QMouseEvent *e) { return false; }
bool CanvasControl::hoverEvent(QHoverEvent *e, Qt::CursorShape *cursor) { return false; }
bool CanvasControl::keyPressEvent(QKeyEvent *e) { return false; }
void CanvasControl::paint(QPainter *painter) {}


