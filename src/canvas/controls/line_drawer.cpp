#include <shape/path_shape.h>
#include <canvas/controls/line_drawer.h>
#include <QPainterPath>
#include <cmath>

bool LineDrawer::mousePressEvent(QMouseEvent *e) {
    CanvasControl::mousePressEvent(e);
    return false;
}

bool LineDrawer::mouseMoveEvent(QMouseEvent *e) {
    if (scene().mode() != Scene::Mode::DRAWING_LINE) return false;
    cursor_ = scene().getCanvasCoord(e->pos());
    return true;
}

bool LineDrawer::mouseReleaseEvent(QMouseEvent *e) {
    if (scene().mode() != Scene::Mode::DRAWING_LINE) return false;
    QPainterPath path;
    path.moveTo(dragged_from_canvas_);
    path.lineTo(scene().getCanvasCoord(e->pos()));
    ShapePtr newLine(new PathShape(path));
    scene().activeLayer().addShape(newLine);
    scene().setSelection(newLine);
    scene().setMode(Scene::Mode::SELECTING);
    return true;
}

void LineDrawer::paint(QPainter *painter){
    if (scene().mode() != Scene::Mode::DRAWING_LINE) return;
    if (cursor_ == QPointF(0,0)) return;
    QPen pen(scene().activeLayer().color(), 3, Qt::SolidLine);
    pen.setCosmetic(true);
    painter->setPen(pen);
    painter->drawLine(dragged_from_canvas_, cursor_);
}

void LineDrawer::reset() {
    cursor_ = QPointF();
}