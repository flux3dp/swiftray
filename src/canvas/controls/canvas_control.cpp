#include <canvas/controls/canvas_control.h>
bool CanvasControl::mousePressEvent(QMouseEvent *e) {
    dragged_from_screen_ = e->pos();
    dragged_from_canvas_ = scene().getCanvasCoord(e->pos());
}
bool CanvasControl::mouseMoveEvent(QMouseEvent *e) {}
bool CanvasControl::mouseReleaseEvent(QMouseEvent *e) {}
bool CanvasControl::hoverEvent(QHoverEvent *e, Qt::CursorShape *cursor) {}
void CanvasControl::paint(QPainter *painter) {}
