#include <shape/path_shape.h>
#include <canvas/controls/oval_drawer.h>
#include <QPainterPath>
#include <cmath>

bool OvalDrawer::mousePressEvent(QMouseEvent *e) {
    CanvasControl::mousePressEvent(e);
    return false;
}

bool OvalDrawer::mouseMoveEvent(QMouseEvent *e) {
    if (scene().mode() != Scene::Mode::DRAWING_OVAL) return false;
    rect_ = QRectF(dragged_from_canvas_, scene().getCanvasCoord(e->pos()));
    return true;
}

bool OvalDrawer::mouseReleaseEvent(QMouseEvent *e) {
    if (scene().mode() != Scene::Mode::DRAWING_OVAL) return false;
    QPainterPath path;
    path.moveTo((rect_.topRight() + rect_.bottomRight()) / 2);
    path.arcTo(rect_, 0, 360 * 16);
    ShapePtr newRect(new PathShape(path));
    scene().activeLayer().addShape(newRect);
    scene().setSelection(newRect);
    scene().setMode(Scene::Mode::SELECTING);
    return true;
}

void OvalDrawer::paint(QPainter *painter){
    if (scene().mode() != Scene::Mode::DRAWING_OVAL) return;
    QPen pen(scene().activeLayer().color(), 3, Qt::SolidLine);
    pen.setCosmetic(true);
    painter->setPen(pen);
    painter->drawArc(rect_, 0, 360 * 16);
}

void OvalDrawer::reset() {
    rect_ = QRectF(0, 0, 0, 0);
}