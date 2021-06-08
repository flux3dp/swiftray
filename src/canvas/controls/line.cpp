#include <QPainterPath>
#include <canvas/controls/line.h>
#include <cmath>
#include <shape/path_shape.h>
using namespace Controls;

bool Line::isActive() { 
    return scene().mode() == Scene::Mode::DRAWING_LINE; 
}

bool Line::mouseMoveEvent(QMouseEvent *e) {
    cursor_ = scene().getCanvasCoord(e->pos());
    return true;
}

bool Line::mouseReleaseEvent(QMouseEvent *e) {
    QPainterPath path;
    path.moveTo(scene().mousePressedCanvasCoord());
    path.lineTo(scene().getCanvasCoord(e->pos()));
    ShapePtr new_line = make_shared<PathShape>(path);
    scene().stackStep();
    scene().activeLayer()->addShape(new_line);
    scene().setSelection(new_line);
    scene().setMode(Scene::Mode::SELECTING);
    return true;
}

void Line::paint(QPainter *painter) {
    if (cursor_ == QPointF(0, 0))
        return;
    QPen pen(scene().activeLayer()->color(), 3, Qt::SolidLine);
    pen.setCosmetic(true);
    painter->setPen(pen);
    painter->drawLine(scene().mousePressedCanvasCoord(), cursor_);
}

void Line::reset() { cursor_ = QPointF(); }