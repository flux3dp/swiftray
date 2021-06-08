#include <QPainterPath>
#include <canvas/controls/rect_drawer.h>
#include <shape/path_shape.h>

bool RectDrawer::isActive() { 
    return scene().mode() == Scene::Mode::DRAWING_RECT; 
}

bool RectDrawer::mouseMoveEvent(QMouseEvent *e) {
    rect_ = QRectF(scene().mousePressedCanvasCoord(), scene().getCanvasCoord(e->pos()));
    return true;
}

bool RectDrawer::mouseReleaseEvent(QMouseEvent *e) {
    QPainterPath path;
    path.addRect(rect_);
    ShapePtr new_rect = make_shared<PathShape>(path);
    scene().stackStep();
    scene().activeLayer().addShape(new_rect);
    scene().setSelection(new_rect);
    scene().setMode(Scene::Mode::SELECTING);
    return true;
}

void RectDrawer::paint(QPainter *painter) {
    QPen pen(scene().activeLayer().color(), 3, Qt::SolidLine);
    pen.setCosmetic(true);
    painter->setPen(pen);
    painter->drawRect(rect_);
}

void RectDrawer::reset() { rect_ = QRectF(0, 0, 0, 0); }