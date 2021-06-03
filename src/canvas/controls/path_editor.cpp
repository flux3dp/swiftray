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
    QPointF local_coord = getLocalCoord(canvas_coord);
    if (dragging_index_ > -1) {
        moveElementTo(dragging_index_, local_coord);

        if (is_closed_shape_) {
            if (dragging_index_ == 0 || dragging_index_ == path().elementCount() - 1) {
                moveElementTo(0, local_coord);
                moveElementTo(path().elementCount() - 1, local_coord);
            }
        }
    }
    return true;
}

void PathEditor::moveElementTo(int index, QPointF local_coord) {
    QPainterPath::Element ele = path().elementAt(index);
    QPointF offset = local_coord - ele;
    int last_index = path().elementCount() - 1;
    qInfo() << "Move element" << index << "Last index" << last_index;
    if (ele.type == QPainterPath::ElementType::CurveToDataElement) {
        if (index > 2 && path().elementAt(index - 2).isCurveTo()) {
            // This element is the curve end point
            QPainterPath::Element prev_ele = path().elementAt(index - 1);
            path().setElementPositionAt(index-1, prev_ele.x + offset.x(), prev_ele.y + offset.y());
            QPainterPath::Element next_ele = path().elementAt((index + 1) % last_index);
            if (next_ele.type == QPainterPath::ElementType::CurveToElement) {
                path().setElementPositionAt((index + 1) % last_index, next_ele.x + offset.x(), next_ele.y + offset.y());
            }
        } else {
            // This element is the control point before an end point
            QPainterPath::Element next_ele_2 = path().elementAt((index + 2) % last_index);
            if (next_ele_2.type == QPainterPath::ElementType::CurveToElement) {
                path().setElementPositionAt((index+2) % last_index, next_ele_2.x - offset.x(), next_ele_2.y - offset.y());
            }
        }
    } else if (ele.type == QPainterPath::ElementType::CurveToElement) {
        // This element is the control point next to an end point
        int prev_index = index - 2 >= 0 ? index - 2 : last_index - 1;
        QPainterPath::Element prev_ele = path().elementAt(prev_index);
        if (prev_ele.type == QPainterPath::ElementType::CurveToDataElement) {
            path().setElementPositionAt(prev_index, prev_ele.x - offset.x(), prev_ele.y - offset.y());
        }
    }
    path().setElementPositionAt(index, local_coord.x(), local_coord.y());
}

bool PathEditor::hoverEvent(QHoverEvent *e, Qt::CursorShape *cursor) {
    if (scene().mode() != Scene::Mode::EDITING_PATH) return false;
    if (target_.get() == nullptr) return false;

    QPointF canvas_coord = scene().getCanvasCoord(e->pos());
    QPointF local_coord = getLocalCoord(canvas_coord);

    if (hitTest(local_coord) > -1) {
        *cursor = Qt::SizeAllCursor;
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
    QPen blue_large_pen(sky_blue, 10, Qt::SolidLine);
    QPen blue_small_pen(sky_blue, 6, Qt::SolidLine);
    blue_pen.setCosmetic(true);
    blue_large_pen.setCosmetic(true);
    blue_small_pen.setCosmetic(true);
    painter->setPen(blue_pen);
    QTransform transform = target_->transform();
    QPolygonF lines;
    QPolygonF large_points;
    QPolygonF small_points;
    for(int i = 0; i < path().elementCount() ; i++ ) {
        QPainterPath::Element ele = path().elementAt(i);
        if (ele.isMoveTo()) {
            large_points << ele;
        } else if (ele.isLineTo()) {
            large_points << ele;
        } else if (ele.isCurveTo()) {
            large_points << path().elementAt(i+2);
            small_points << ele;
            small_points << path().elementAt(i+1);
            lines << path().elementAt(i+1);
            lines << path().elementAt(i+2);
            lines << ele;
            lines << path().elementAt(i-1);
        }
    }
    painter->setPen(blue_large_pen);
    painter->drawPoints(transform.map(large_points));
    painter->setPen(blue_small_pen);
    painter->drawPoints(transform.map(small_points));
    painter->setPen(blue_pen);
    painter->drawLines(transform.map(lines));
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