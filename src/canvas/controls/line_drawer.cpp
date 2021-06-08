#include <QPainterPath>
#include <canvas/controls/line_drawer.h>
#include <cmath>
#include <shape/path_shape.h>

bool LineDrawer::isActive() { 
    return scene().mode() == Scene::Mode::DRAWING_LINE; 
}

bool LineDrawer::mouseMoveEvent(QMouseEvent *e) {
    cursor_ = scene().getCanvasCoord(e->pos());
    return true;
}

bool LineDrawer::mouseReleaseEvent(QMouseEvent *e) {
    QPainterPath path;
    path.moveTo(scene().mousePressedCanvasCoord());
    path.lineTo(scene().getCanvasCoord(e->pos()));
    ShapePtr new_line = make_shared<PathShape>(path);
    scene().stackStep();
    scene().activeLayer().addShape(new_line);
    scene().setSelection(new_line);
    scene().setMode(Scene::Mode::SELECTING);
    return true;
}

void LineDrawer::paint(QPainter *painter) {
    if (cursor_ == QPointF(0, 0))
        return;
    QPen pen(scene().activeLayer().color(), 3, Qt::SolidLine);
    pen.setCosmetic(true);
    painter->setPen(pen);
    painter->drawLine(scene().mousePressedCanvasCoord(), cursor_);
}

void LineDrawer::reset() { cursor_ = QPointF(); }