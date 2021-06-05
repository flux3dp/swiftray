#include <canvas/controls/transform_box.h>
#include <QDebug>
#include <QObject>

TransformBox::TransformBox(Scene &scene) noexcept : CanvasControl(scene) {
    active_control_ = Control::NONE;
    connect((QObject*)(&this->scene()), SIGNAL(selectionsChanged()), this, SLOT(updateSelections()));
}

QList<ShapePtr> &TransformBox::selections() {
    return selections_;
}

void TransformBox::updateSelections() {
    selections().clear();
    selections().append(scene().selections());
    bbox_need_recalc_ = true;
}

void TransformBox::updateBoundingRect() {
    // Check if all selection's rotation are the same
    bool all_same_rotation = true;
    qreal rotation = selections().size() > 0 ? selections().first()->rotation() : 0;
    
    for(ShapePtr &selection : selections()) {
        if (selection->rotation() != rotation) {
            all_same_rotation = false;
            break;
        }
    }

    float top = std::numeric_limits<float>::max();
    float bottom = std::numeric_limits<float>::min();
    float left = std::numeric_limits<float>::max();
    float right = std::numeric_limits<float>::min();

    for (int i = 0; i < selections().size(); i++) {
        // TODO:: need to add all same rotation scenario
        QRectF bb = selections().at(i)->boundingRect();
        if (all_same_rotation) bb = selections().at(i)->unrotated_bbox_;

        if (bb.left() < left) left = bb.left();
        if (bb.top() < top) top = bb.top();
        if (bb.right() > right) right = bb.right();
        if (bb.bottom() > bottom) bottom = bb.bottom();
    }

    bounding_rect_ = QRectF(left, top, right - left, bottom - top);
    bbox_angle_ = all_same_rotation ? rotation : 0;
}

QRectF TransformBox::boundingRect() {
    if (bbox_need_recalc_) {
        updateBoundingRect();
        bbox_need_recalc_ = false;
    }
    return bounding_rect_;
}

void TransformBox::applyRotate(bool temporarily) {
    if (selections().size() == 0) return;

    QTransform transform = QTransform().translate(action_center_.x(), action_center_.y())
                                .rotate(rotation_to_apply_)
                                .translate(-action_center_.x(), -action_center_.y());

    for (ShapePtr &shape : selections()) {
        if (temporarily) {
            shape->temp_transform_ = transform;
        } else {
            shape->temp_transform_ = QTransform();
            shape->applyTransform(transform);
            shape->setRotation(shape->rotation() + rotation_to_apply_);
        }
    }

    if (!temporarily) {
        rotation_to_apply_ = 0;
        bbox_need_recalc_ = true;
        emit transformChanged();
    }
}

void TransformBox::applyScale(bool temporarily) {
    if (selections().size() == 0) return;

    QTransform transform = QTransform().translate(action_center_.x(), action_center_.y())
                                .rotate(bbox_angle_)
                                .scale(scale_x_to_apply_, scale_y_to_apply_)
                                .rotate(-bbox_angle_)
                                .translate(-action_center_.x(), -action_center_.y());

    for (ShapePtr &shape : selections()) {
        if (temporarily) {
            shape->temp_transform_ = transform;
        } else {
            shape->temp_transform_ = QTransform();
            qInfo() << "Applying scale" << scale_x_to_apply_ << scale_y_to_apply_;
            shape->applyTransform(transform);
        }
    }

    if (!temporarily) {
        scale_x_to_apply_ = scale_y_to_apply_ = 1;
        bbox_need_recalc_ = true;
        emit transformChanged();
    }
}

void TransformBox::applyMove(bool temporarily) {
    if (selections().size() == 0) return;

    QTransform transform = QTransform().translate(translate_to_apply_.x(), translate_to_apply_.y());

    for (ShapePtr &shape : selections()) {
        if (temporarily) {
            shape->temp_transform_ = transform;
        } else {
            shape->temp_transform_ = QTransform();
            shape->applyTransform(transform);
        }
    }

    if (!temporarily) {
        bbox_need_recalc_ = true;
        translate_to_apply_ = QPointF();
        emit transformChanged();
    }
}

const QPointF *TransformBox::controlPoints() {
    QRectF bbox = boundingRect();
    QTransform rotate_transform = QTransform().translate(bbox.center().x(), bbox.center().y())
                                            .rotate(bbox_angle_ + rotation_to_apply_)
                                            .translate(-bbox.center().x(), -bbox.center().y());
    controls_[0] = rotate_transform.map(bbox.topLeft());
    controls_[1] = rotate_transform.map((bbox.topLeft() + bbox.topRight()) / 2);
    controls_[2] = rotate_transform.map(bbox.topRight());
    controls_[3] = rotate_transform.map((bbox.topRight() + bbox.bottomRight()) / 2);
    controls_[4] = rotate_transform.map(bbox.bottomRight());
    controls_[5] = rotate_transform.map((bbox.bottomRight() + bbox.bottomLeft()) / 2);
    controls_[6] = rotate_transform.map(bbox.bottomLeft());
    controls_[7] = rotate_transform.map((bbox.bottomLeft() + bbox.topLeft()) / 2);
    return controls_;
}

TransformBox::Control TransformBox::hitTest(QPointF clickPoint, float tolerance) {
    controlPoints();
    for (int i = (int)Control::NW ; i != (int)Control::ROTATION; i++) {
        if ((controls_[i] - clickPoint).manhattanLength() < tolerance) {
            return (Control)i;
        }
    }

    if (!this->boundingRect().contains(clickPoint)) {
        for (int i = 0; i < 8; i++) {
            if ((controls_[i] - clickPoint).manhattanLength() < tolerance * 2) {
                return Control::ROTATION;
            }
        }
    }

    return Control::NONE;
}

bool TransformBox::mousePressEvent(QMouseEvent *e) {
    if (scene().mode() == Scene::Mode::EDITING_PATH) return false;
    if (scene().mode() == Scene::Mode::DRAWING_TEXT) return false;

    CanvasControl::mousePressEvent(e);
    QPointF canvas_coord = scene().getCanvasCoord(e->pos());

    active_control_ = hitTest(canvas_coord, 10 / scene().scale());
    if (active_control_ == Control::NONE) return false;

    scale_x_to_apply_ = scale_y_to_apply_ = 1;
    rotation_to_apply_ = 0;
    translate_to_apply_ = QPointF();
    flipped_x = flipped_y = false;

    QRectF bbox = boundingRect();

    if (active_control_ == Control::ROTATION) {
        action_center_ = bbox.center();
        rotated_from_ = atan2(canvas_coord.y() - action_center_.y(), canvas_coord.x() - action_center_.x());
        qInfo() << "Transform rotation" << rotated_from_;
        scene().setMode(Scene::Mode::ROTATING);
    } else {
        action_center_ = controls_[((int)active_control_ + 4) % 8];
        transformed_from_ = QSizeF(bbox.size());
        scene().setMode(Scene::Mode::TRANSFORMING);
    }
    scene().stackStep();
    return true;
}

bool TransformBox::mouseReleaseEvent(QMouseEvent *e) {
    if (scene().mode() == Scene::Mode::EDITING_PATH) return false;
    if (active_control_ == Control::NONE && scene().mode() != Scene::Mode::MOVING) return false;

    if (rotation_to_apply_ != 0) applyRotate(false);
    if (translate_to_apply_ != QPointF()) applyMove(false);
    if (scale_x_to_apply_ != 1 || scale_y_to_apply_ != 1) applyScale(false);

    active_control_ = Control::NONE;
    flipped_x = flipped_y = false;

    scene().setMode(Scene::Mode::SELECTING);
    return true;
}

void TransformBox::calcScale(QPointF canvas_coord) {
    cursor_ = canvas_coord;
    QRectF bbox = boundingRect();
    QTransform local_transform_invert = QTransform().translate(bbox.center().x(), bbox.center().y()).rotate(bbox_angle_).inverted();
    QPointF local_coord = local_transform_invert.map(canvas_coord);

    QPointF d = local_coord - local_transform_invert.map(action_center_);

    qInfo() << "Local coord" << local_coord << "BBox center" << bbox.center() << "Action center" << action_center_;

    if (active_control_ == Control::N || 
        active_control_ == Control::NE || 
        active_control_ == Control::NW) d.setY(-d.y());
    if (active_control_ == Control::NW || 
        active_control_ == Control::W) d.setX(-d.x());

    scale_x_to_apply_ = d.x() / transformed_from_.width();
    scale_y_to_apply_ = d.y() / transformed_from_.height();

    if (active_control_ == Control::N || active_control_ == Control::S) {
        scale_x_to_apply_ = 1;
    } else if (active_control_ == Control::W || active_control_ == Control::E) {
        scale_y_to_apply_ = 1;
    }
}

bool TransformBox::mouseMoveEvent(QMouseEvent *e) {
    if (scene().mode() == Scene::Mode::EDITING_PATH) return false;
    if (scene().selections().size() == 0) return false;

    QPointF canvas_coord = scene().getCanvasCoord(e->pos());

    // Todo - check if no selection
    if (scene().mode() == Scene::Mode::SELECTING) {
        if ((e->pos() - dragged_from_screen_).manhattanLength() > 3) {
            scene().stackStep();
            scene().setMode(Scene::Mode::MOVING);
        }
    }

    switch(scene().mode()) {
        case Scene::Mode::MOVING:
            translate_to_apply_ = canvas_coord - dragged_from_canvas_;
            applyMove(true);
            break;
        case Scene::Mode::ROTATING:
            rotation_to_apply_ = (atan2(canvas_coord.y() - action_center_.y(), canvas_coord.x() - action_center_.x()) - rotated_from_) * 180 / 3.1415926;
            qInfo() << "Transform rotation" << rotation_to_apply_ << action_center_;
            applyRotate(true);
            break;
        
        case Scene::Mode::TRANSFORMING:
            calcScale(canvas_coord);
            applyScale(true);
            break;
        default:
            return false;
    }
    return true;
}

bool TransformBox::hoverEvent(QHoverEvent *e, Qt::CursorShape *cursor) {
    if (scene().mode() == Scene::Mode::EDITING_PATH) return false;
    if (scene().mode() == Scene::Mode::DRAWING_TEXT) return false;
    Control cp = hitTest(scene().getCanvasCoord(e->pos()), 10 / scene().scale());

    switch (cp) {
    case Control::ROTATION:
        *cursor = Qt::CrossCursor;
        return true;

    case Control::NW:
    case Control::SE:
        *cursor = Qt::SizeFDiagCursor;
        return true;

    case Control::NE:
    case Control::SW:
        *cursor = Qt::SizeBDiagCursor;
        return true;

    case Control::N:
    case Control::S:
        *cursor = Qt::SizeVerCursor;
        return true;

    case Control::W:
    case Control::E:
        *cursor = Qt::SizeHorCursor;
        return true;

    default:
        break;
    }

    return false;
}

void TransformBox::paint(QPainter *painter) {
    if (scene().mode() == Scene::Mode::EDITING_PATH) return;
    if (scene().mode() == Scene::Mode::DRAWING_TEXT) return;
    auto sky_blue = QColor::fromRgb(0x00, 0x99, 0xCC, 255);
    auto blue_pen = QPen(QBrush(sky_blue), 0, Qt::DashLine);
    auto pt_pen = QPen(sky_blue, 10 / scene().scale(), Qt::PenStyle::SolidLine, Qt::RoundCap);

    if (selections().size() > 0) {
        if (active_control_ == Control::ROTATION) {
            painter->save();
            painter->translate(action_center_);
            painter->setPen(blue_pen);
            painter->rotate(bbox_angle_ + rotation_to_apply_);
            painter->drawRect(boundingRect().translated(-boundingRect().center()));
            painter->restore();
        } else {
            painter->setPen(blue_pen);
            painter->drawPolyline(controlPoints(), 8);
            painter->drawLine(controlPoints()[7], controlPoints()[0]);
            painter->setPen(pt_pen);
            painter->drawPoints(controlPoints(), 8);
            painter->drawPoint(boundingRect().center());
        }
    }
}
