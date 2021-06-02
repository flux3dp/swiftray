#include <shape/path_shape.h>
#include <canvas/controls/path_editor.h>
#include <QPainterPath>
#include <QApplication>
#include <cmath>
#include <cfloat>
#include <QDebug>

PathEditor::PathEditor(Scene &scene_) noexcept: CanvasControl(scene_) {
    dragging_index_ = -1;
    target_ = nullptr;
}

bool PathEditor::mousePressEvent(QMouseEvent *e) {
    if (scene().mode() != Scene::Mode::EDITING_PATH) return false;
    if (target_.get() == nullptr) return false;
    CanvasControl::mousePressEvent(e);
    QPointF canvas_coord = scene().getCanvasCoord(e->pos());
    dragging_index_ = hitTest(getLocalCoord(canvas_coord));
    if (dragging_index_ > -1) {
        return true;
    } else {
        return false;
    }
}

bool PathEditor::mouseMoveEvent(QMouseEvent *e) {
    if (scene().mode() != Scene::Mode::EDITING_PATH) return false;
    if (target_.get() == nullptr) return false;

    QPointF canvas_coord = scene().getCanvasCoord(e->pos());
    if (dragging_index_ > -1) {
        QPointF local_coord = getLocalCoord(canvas_coord);
        path().setElementPositionAt(dragging_index_, local_coord.x(), local_coord.y());

        if (is_closed_shape_) {
            if (dragging_index_ == 0 || dragging_index_ == path().elementCount() - 1) {
                path().setElementPositionAt(0, local_coord.x(), local_coord.y());
                path().setElementPositionAt(path().elementCount() - 1, local_coord.x(), local_coord.y());
            }
        } 
    }
    return true;
}

bool PathEditor::hoverEvent(QHoverEvent *e, Qt::CursorShape *cursor) {
    if (scene().mode() != Scene::Mode::EDITING_PATH) return false;
    if (target_.get() == nullptr) return false;

    QPointF canvas_coord = scene().getCanvasCoord(e->pos());
    QPointF local_coord = getLocalCoord(canvas_coord);

    if (hitTest(local_coord) > -1) {
        qInfo() << "Hover" << hitTest(local_coord);
        *cursor = Qt::DragMoveCursor;
        return true;
    } else {
        return false;
    }
}

bool PathEditor::mouseReleaseEvent(QMouseEvent *e) {
    if (scene().mode() != Scene::Mode::EDITING_PATH) return false;
    if (target_.get() == nullptr) return false;
    QPointF canvas_coord = scene().getCanvasCoord(e->pos());
    
    return true;
}

int PathEditor::hitTest(QPointF local_coord) {
    if (target_.get() == nullptr) return -1;

    for(int i = 0; i < path().elementCount() ; i++ ) {
        QPainterPath::Element ele = path().elementAt(i);
        if ((ele - local_coord).manhattanLength() < 15 / scene().scale()) {
            return i;
        }
    }
    return -1;
}

void PathEditor::paint(QPainter *painter){
    if (scene().mode() != Scene::Mode::EDITING_PATH) return;
    if (target_.get() == nullptr) return;

    QColor sky_blue = QColor::fromRgb(0x00, 0x99, 0xCC, 255);
    QPen blue_pen(sky_blue, 2, Qt::SolidLine);
    blue_pen.setCosmetic(true);
    painter->setBrush(QBrush(Qt::white,Qt::SolidPattern));
    painter->setPen(blue_pen);
    painter->save();
    QTransform transform = target_->transform();
    for(int i = 0; i < path().elementCount() ; i++ ) {
        QPainterPath::Element ele = path().elementAt(i);
        if (ele.isMoveTo()) {
            painter->drawEllipse(transform.map(ele), 5, 5);
        } else if (ele.isLineTo()) {
            painter->drawEllipse(transform.map(ele), 5, 5);
        } else if (ele.isCurveTo()) {
            painter->drawEllipse(transform.map(ele), 3, 3);
            painter->drawEllipse(transform.map(path().elementAt(i+1)), 3, 3);
            painter->drawEllipse(transform.map(path().elementAt(i+2)), 5, 5);
        }
    }
    painter->restore();
    painter->setBrush(Qt::NoBrush);
}

void PathEditor::reset() {
    target_ = nullptr;
}

QPainterPath &PathEditor::path() {
    PathShape *path_shape_ = (PathShape*)target_.get();
    return path_shape_->path_;
}


QPointF PathEditor::getLocalCoord(QPointF canvas_coord) {
    return ((PathShape*)target_.get())->transform().inverted().map(canvas_coord);
}

ShapePtr PathEditor::target() {
    return target_;
}

void PathEditor::setTarget(ShapePtr target) {
    target_ = target;
    int elem_count = path().elementCount();
    is_closed_shape_ = elem_count > 0 && 
                      (path().elementAt(0) - path().elementAt(elem_count - 1)).manhattanLength() <= FLT_EPSILON;
}