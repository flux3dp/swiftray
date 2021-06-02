#include <shape/path_shape.h>
#include <canvas/controls/rect_drawer.h>
#include <QPainterPath>

bool RectDrawer::mousePressEvent(QMouseEvent *e) {
    CanvasControl::mousePressEvent(e);
    return false;
}

bool RectDrawer::mouseMoveEvent(QMouseEvent *e) {
    if (scene().mode() != Scene::Mode::DRAWING_RECT) return false;
    rect_ = QRectF(dragged_from_canvas_, scene().getCanvasCoord(e->pos()));
    return true;
}

bool RectDrawer::mouseReleaseEvent(QMouseEvent *e) {
    if (scene().mode() != Scene::Mode::DRAWING_RECT) return false;
    QPainterPath path;
    path.addRect(rect_);
    ShapePtr newRect(new PathShape(path));
    scene().activeLayer().addShape(newRect);
    scene().setSelection(newRect);
    scene().setMode(Scene::Mode::SELECTING);
    return true;
}

void RectDrawer::paint(QPainter *painter){
    if (scene().mode() != Scene::Mode::DRAWING_RECT) return;
    QPen pen(scene().activeLayer().color(), 3, Qt::SolidLine);
    pen.setCosmetic(true);
    painter->setPen(pen);
    painter->drawRect(rect_);
}

void RectDrawer::reset() {
    rect_ = QRectF(0, 0, 0, 0);
}