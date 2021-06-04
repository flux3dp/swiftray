#include <canvas/controls/canvas_control.h>
CanvasControl::CanvasControl(Scene &scene) : scene_(scene) {

};

Scene &CanvasControl::scene() {
    return scene_;
}

bool CanvasControl::mousePressEvent(QMouseEvent *e) {
    dragged_from_screen_ = e->pos();
    dragged_from_canvas_ = scene().getCanvasCoord(e->pos());
}

bool CanvasControl::mouseMoveEvent(QMouseEvent *e) {}
bool CanvasControl::mouseReleaseEvent(QMouseEvent *e) {}
bool CanvasControl::hoverEvent(QHoverEvent *e, Qt::CursorShape *cursor) {}
bool CanvasControl::keyPressEvent(QKeyEvent *e) {}
void CanvasControl::paint(QPainter *painter) {}


