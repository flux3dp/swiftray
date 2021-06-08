#include <QPainterPath>
#include <canvas/controls/oval_drawer.h>
#include <cmath>
#include <shape/path_shape.h>

bool OvalDrawer::isActive() { 
    return scene().mode() == Scene::Mode::DRAWING_OVAL; 
}

bool OvalDrawer::mouseMoveEvent(QMouseEvent *e) {
    rect_ = QRectF(scene().mousePressedCanvasCoord(), scene().getCanvasCoord(e->pos()));
    return true;
}

bool OvalDrawer::mouseReleaseEvent(QMouseEvent *e) {
    QPainterPath path;
    path.moveTo((rect_.topRight() + rect_.bottomRight()) / 2);
    path.arcTo(rect_, 0, 360 * 16);
    ShapePtr new_oval = make_shared<PathShape>(path);
    scene().stackStep();
    scene().activeLayer().addShape(new_oval);
    scene().setSelection(new_oval);
    scene().setMode(Scene::Mode::SELECTING);
    return true;
}

void OvalDrawer::paint(QPainter *painter) {
    QPen pen(scene().activeLayer().color(), 3, Qt::SolidLine);
    pen.setCosmetic(true);
    painter->setPen(pen);
    painter->drawArc(rect_, 0, 360 * 16);
}

void OvalDrawer::reset() { rect_ = QRectF(0, 0, 0, 0); }