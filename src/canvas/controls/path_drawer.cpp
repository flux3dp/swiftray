#include <QApplication>
#include <QDebug>
#include <QPainterPath>
#include <canvas/controls/path_drawer.h>
#include <cmath>
#include <shape/path_shape.h>

bool PathDrawer::isActive() { 
    return scene().mode() == Scene::Mode::DRAWING_PATH; 
}

PathDrawer::PathDrawer(Scene &scene_) noexcept : CanvasControl(scene_) {
    curve_target_ = invalid_point;
    last_ctrl_pt_ = invalid_point;
    is_drawing_curve_ = false;
    is_closing_curve_ = false;
}

bool PathDrawer::mousePressEvent(QMouseEvent *e) {
    QPointF canvas_coord = scene().getCanvasCoord(e->pos());

    curve_target_ = canvas_coord;

    if (hitOrigin(canvas_coord)) {
        curve_target_ = working_path_.elementAt(0);
    }
    if (hitTest(canvas_coord)) {
        is_closing_curve_ = true;
    }
    return true;
}

bool PathDrawer::mouseMoveEvent(QMouseEvent *e) {
    if (scene().mode() != Scene::Mode::DRAWING_PATH)
        return false;
    QPointF canvas_coord = scene().getCanvasCoord(e->pos());
    if ((canvas_coord - curve_target_).manhattanLength() < 10)
        return false;
    is_drawing_curve_ = true;
    cursor_ = scene().getCanvasCoord(e->pos());
    return true;
}

bool PathDrawer::hoverEvent(QHoverEvent *e, Qt::CursorShape *cursor) {
    if (scene().mode() != Scene::Mode::DRAWING_PATH)
        return false;
    *cursor = Qt::CrossCursor;
    cursor_ = scene().getCanvasCoord(e->pos());
    return true;
}

bool PathDrawer::mouseReleaseEvent(QMouseEvent *e) {
    if (scene().mode() != Scene::Mode::DRAWING_PATH)
        return false;
    QPointF canvas_coord = scene().getCanvasCoord(e->pos());

    if (working_path_.elementCount() == 0) {
        working_path_.moveTo(canvas_coord);
        last_ctrl_pt_ = canvas_coord;
        return true;
    }

    if (!hitTest(canvas_coord) || hitOrigin(canvas_coord)) {
        if (is_drawing_curve_) {
            if (curve_target_ != invalid_point) {
                if (last_ctrl_pt_ != invalid_point) {
                    working_path_.cubicTo(last_ctrl_pt_,
                                          curve_target_ * 2 - cursor_,
                                          curve_target_);
                } else {
                    working_path_.cubicTo(curve_target_ * 2 - cursor_,
                                          curve_target_ * 2 - cursor_,
                                          curve_target_);
                }
                last_ctrl_pt_ = cursor_;
                curve_target_ = invalid_point;
            } else {
                qInfo() << "Release"
                        << "Write Curve Point 2";
                working_path_.cubicTo(last_ctrl_pt_, cursor_, cursor_);
                last_ctrl_pt_ = invalid_point;
                is_drawing_curve_ = false;
            }
        } else {
            qInfo() << "Release"
                    << "Write Line Point";
            working_path_.lineTo(canvas_coord);
        }
    }

    if (is_closing_curve_) {
        ShapePtr new_shape = make_shared<PathShape>(working_path_);
        scene().stackStep();
        scene().activeLayer().addShape(new_shape);
        scene().setSelection(new_shape);
        scene().setMode(Scene::Mode::SELECTING);
        reset();
    }
    return true;
}

bool PathDrawer::hitOrigin(QPointF canvas_coord) {
    if (working_path_.elementCount() > 0 &&
        (working_path_.elementAt(0) - canvas_coord).manhattanLength() <
            15 / scene().scale()) {
        return true;
    }
    return false;
}

bool PathDrawer::hitTest(QPointF canvas_coord) {
    for (int i = 0; i < working_path_.elementCount(); i++) {
        QPainterPath::Element ele = working_path_.elementAt(i);
        if (ele.isMoveTo()) {
            if ((ele - canvas_coord).manhattanLength() < 15 / scene().scale()) {
                return true;
            }
        } else if (ele.isLineTo()) {
            if ((ele - canvas_coord).manhattanLength() < 15 / scene().scale()) {
                return true;
            }
        } else if (ele.isCurveTo()) {
            QPointF ele_end_point = working_path_.elementAt(i + 2);
            if ((ele_end_point - canvas_coord).manhattanLength() <
                15 / scene().scale()) {
                return true;
            }
        }
    }
    return false;
}

void PathDrawer::paint(QPainter *painter) {
    if (scene().mode() != Scene::Mode::DRAWING_PATH)
        return;
    auto sky_blue = QColor::fromRgb(0x00, 0x99, 0xCC, 255);
    auto blue_pen = QPen(sky_blue, 2, Qt::SolidLine);
    auto black_pen = QPen(scene().activeLayer().color(), 3, Qt::SolidLine);
    blue_pen.setCosmetic(true);
    black_pen.setCosmetic(true);
    painter->setPen(black_pen);
    if (working_path_.elementCount() > 0) {
        if (is_drawing_curve_) {
            QPainterPath wp_clone = working_path_;
            if (curve_target_ != invalid_point) {
                wp_clone.cubicTo(last_ctrl_pt_, curve_target_ * 2 - cursor_,
                                 curve_target_);
                painter->drawPath(wp_clone);
                painter->setPen(blue_pen);
                painter->drawLine(cursor_, curve_target_);
                painter->drawLine(curve_target_ * 2 - cursor_, curve_target_);
                painter->drawEllipse(curve_target_ * 2 - cursor_, 3, 3);
                painter->drawEllipse(curve_target_, 5, 5);
                painter->drawEllipse(cursor_, 3, 3);
            } else {
                QPainterPath wp_clone = working_path_;
                wp_clone.cubicTo(last_ctrl_pt_, cursor_, cursor_);
                painter->drawPath(wp_clone);
            }
        } else {
            painter->drawPath(working_path_);
            painter->drawLine(working_path_.pointAtPercent(1), cursor_);
        }
    }

    painter->setPen(blue_pen);
    for (int i = 0; i < working_path_.elementCount(); i++) {
        QPainterPath::Element ele = working_path_.elementAt(i);
        if (ele.isMoveTo()) {
            painter->drawEllipse(ele, 5, 5);
        } else if (ele.isLineTo()) {
            painter->drawEllipse(ele, 5, 5);
        } else if (ele.isCurveTo()) {
            QPointF ele_end_point = working_path_.elementAt(i + 2);
            painter->drawEllipse(ele_end_point, 5, 5);
        }
    }
}

void PathDrawer::reset() {
    working_path_ = QPainterPath();
    curve_target_ = invalid_point;
    last_ctrl_pt_ = invalid_point;
    is_drawing_curve_ = false;
    is_closing_curve_ = false;
}