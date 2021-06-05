#include <canvas/controls/transform_box.h>
#include <QDebug>
#include <QObject>

TransformBox::TransformBox(Scene &scene) noexcept : CanvasControl(scene) {
    activating_control_ = ControlPoint::NONE;
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
        if (all_same_rotation) {
            bb = selections().at(i)->unrotated_bbox_;
            qInfo() << "Unrotated bbox" << bb;
        }

        if (bb.left() < left) {
            left = bb.left();
        }

        if (bb.top() < top) {
            top = bb.top();
        }

        if (bb.right() > right) {
            right = bb.right();
        }

        if (bb.bottom() > bottom) {
            bottom = bb.bottom();
        }
    }

    bounding_rect_ = QRectF(left, top, right - left, bottom - top);
    bbox_rotation_ = all_same_rotation ? rotation : 0;
    qInfo() << "Update bouding rect" << bounding_rect_ << "rotation" << bbox_rotation_;
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
                                .rotate(bbox_rotation_)
                                .scale(scale_x_to_apply_, scale_y_to_apply_)
                                .rotate(-bbox_rotation_)
                                .translate(-action_center_.x(), -action_center_.y());

    for (ShapePtr &shape : selections()) {
        if (temporarily) {
            shape->temp_transform_ = transform;
        } else {
            shape->temp_transform_ = QTransform();
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
                                            .rotate(bbox_rotation_ + rotation_to_apply_)
                                            .translate(-bbox.center().x(), -bbox.center().y());
    control_points_[0] = rotate_transform.map(bbox.topLeft());
    control_points_[1] = rotate_transform.map((bbox.topLeft() + bbox.topRight()) / 2);
    control_points_[2] = rotate_transform.map(bbox.topRight());
    control_points_[3] = rotate_transform.map((bbox.topRight() + bbox.bottomRight()) / 2);
    control_points_[4] = rotate_transform.map(bbox.bottomRight());
    control_points_[5] = rotate_transform.map((bbox.bottomRight() + bbox.bottomLeft()) / 2);
    control_points_[6] = rotate_transform.map(bbox.bottomLeft());
    control_points_[7] = rotate_transform.map((bbox.bottomLeft() + bbox.topLeft()) / 2);
    return control_points_;
}

TransformBox::ControlPoint TransformBox::hitTest(QPointF clickPoint, float tolerance) {
    controlPoints();
    for (int i = (int)ControlPoint::NW ; i != (int)ControlPoint::ROTATION; i++) {
        if ((control_points_[i] - clickPoint).manhattanLength() < tolerance) {
            return (ControlPoint)i;
        }
    }

    if (!this->boundingRect().contains(clickPoint)) {
        for (int i = 0; i < 8; i++) {
            if ((control_points_[i] - clickPoint).manhattanLength() < tolerance * 2) {
                return ControlPoint::ROTATION;
            }
        }
    }

    return ControlPoint::NONE;
}

bool TransformBox::mousePressEvent(QMouseEvent *e) {
    if (scene().mode() == Scene::Mode::EDITING_PATH) return false;
    if (scene().mode() == Scene::Mode::DRAWING_TEXT) return false;

    scale_x_to_apply_ = 1;
    scale_y_to_apply_ = 1;
    rotation_to_apply_ = 0;
    translate_to_apply_ = QPointF();

    CanvasControl::mousePressEvent(e);
    QPointF canvas_coord = scene().getCanvasCoord(e->pos());
    activating_control_ = hitTest(canvas_coord, 10 / scene().scale());

    switch (activating_control_) {
    case ControlPoint::ROTATION:
        action_center_ = boundingRect().center();
        rotated_from = atan2(canvas_coord.y() - action_center_.y(), canvas_coord.x() - action_center_.x());
        init_rotation_rect_ = boundingRect().translated(-action_center_);
        qInfo() << "Transform rotation" << rotated_from;
        scene().stackStep();
        scene().setMode(Scene::Mode::ROTATING);
        return true;

    case ControlPoint::NW:
        action_center_ = control_points_[(int)ControlPoint::SE];
        break;

    case ControlPoint::NE:
        action_center_ = control_points_[(int)ControlPoint::SW];
        break;

    case ControlPoint::SW:
        action_center_ = control_points_[(int)ControlPoint::NE];
        break;

    case ControlPoint::SE:
        action_center_ = control_points_[(int)ControlPoint::NW];
        break;

    case ControlPoint::N:
        action_center_ = control_points_[(int)ControlPoint::S];
        break;
    case ControlPoint::W:
        action_center_ = control_points_[(int)ControlPoint::E];
        break;
    case ControlPoint::S:
        action_center_ = control_points_[(int)ControlPoint::N];
        break;
    case ControlPoint::E:
        action_center_ = control_points_[(int)ControlPoint::W];
        break;

    default:
        return false;
    }

    transformed_from_ = QSizeF(boundingRect().size());
    flipped_x = flipped_y = false;
    scene().stackStep();
    scene().setMode(Scene::Mode::TRANSFORMING);
    return true;
}

bool TransformBox::mouseReleaseEvent(QMouseEvent *e) {
    if (scene().mode() == Scene::Mode::EDITING_PATH) return false;
    if (activating_control_ == ControlPoint::NONE && scene().mode() != Scene::Mode::MOVING) return false;

    if (rotation_to_apply_ != 0) applyRotate(false);
    if (translate_to_apply_ != QPointF()) applyMove(false);
    if (scale_x_to_apply_ != 0 || scale_y_to_apply_ != 0) applyScale(false);

    activating_control_ = ControlPoint::NONE;
    flipped_x = flipped_y = false;

    scene().setMode(Scene::Mode::SELECTING);
    return true;
}

void TransformBox::calcScale(QPointF canvas_coord) {
    cursor_ = canvas_coord;
    QRectF bbox = boundingRect();
    QTransform local_transform_invert = QTransform().translate(bbox.center().x(), bbox.center().y()).rotate(bbox_rotation_).inverted();
    QPointF local_coord = local_transform_invert.map(canvas_coord);

    QPointF d = local_coord - local_transform_invert.map(action_center_);

    qInfo() << "Local coord" << local_coord << "BBox center" << bbox.center() << "Action center" << action_center_;

    switch (activating_control_) {
    case ControlPoint::N:
    case ControlPoint::NE:
        d.setY(-d.y());
        break;

    case ControlPoint::NW:
        d.setY(-d.y());
        d.setX(-d.x());
        break;

    case ControlPoint::W:
        d.setX(-d.x());
        break;
    }
    qreal sx = d.x() / transformed_from_.width();
    qreal sy = d.y() / transformed_from_.height();

    if (activating_control_ == ControlPoint::N || activating_control_ == ControlPoint::S) {
        sx = 1;
    } else if (activating_control_ == ControlPoint::W || activating_control_ == ControlPoint::E) {
        sy = 1;
    }

    if (sx > 0 && flipped_x) {
        flipped_x = false;
        sx = -sx;
    }

    if (sy > 0 && flipped_y) {
        flipped_y = false;
        sy = -sy;
    }

    if (flipped_x) sx = abs(sx);

    if (flipped_y) sy = abs(sy);

    if (sx < 0) flipped_x = true;

    if (sy < 0) flipped_y = true;

    qInfo() << "DXDY" << d.x() << d.y();

    scale_x_to_apply_ = sx;
    scale_y_to_apply_ = sy;
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
            rotation_to_apply_ = (atan2(canvas_coord.y() - action_center_.y(), canvas_coord.x() - action_center_.x()) - rotated_from) * 180 / 3.1415926;
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
    ControlPoint cp = hitTest(scene().getCanvasCoord(e->pos()), 10 / scene().scale());

    switch (cp) {
    case ControlPoint::ROTATION:
        *cursor = Qt::CrossCursor;
        return true;

    case ControlPoint::NW:
    case ControlPoint::SE:
        *cursor = Qt::SizeFDiagCursor;
        return true;

    case ControlPoint::NE:
    case ControlPoint::SW:
        *cursor = Qt::SizeBDiagCursor;
        return true;

    case ControlPoint::N:
    case ControlPoint::S:
        *cursor = Qt::SizeVerCursor;
        return true;

    case ControlPoint::W:
    case ControlPoint::E:
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
    QColor sky_blue = QColor::fromRgb(0x00, 0x99, 0xCC, 255);
    QPen bluePen = QPen(QBrush(sky_blue), 0, Qt::DashLine);
    QPen greenPen = QPen(Qt::green, 6, Qt::DotLine);
    QPen ptPen(sky_blue, 10 / scene().scale(), Qt::PenStyle::SolidLine, Qt::RoundCap);

    if (selections().size() > 0) {
        if (activating_control_ == ControlPoint::ROTATION) {
            painter->save();
            painter->translate(action_center_);
            painter->setPen(bluePen);
            painter->rotate(bbox_rotation_ + rotation_to_apply_);
            painter->drawRect(init_rotation_rect_);
            painter->restore();
        } else {
            painter->setPen(bluePen);
            painter->drawPolyline(controlPoints(), 8);
            painter->drawLine(controlPoints()[7], controlPoints()[0]);
            painter->setPen(ptPen);
            painter->drawPoints(controlPoints(), 8);
            painter->drawPoint(boundingRect().center());
            // Draw unrotated bouding rect
            QRectF bbox = boundingRect();
            QTransform trans = QTransform().translate(boundingRect().center().x(), boundingRect().center().y()).rotate(bbox_rotation_);
            painter->save();
            painter->setPen(greenPen);
            painter->setTransform(trans, true);
            painter->drawRect(QRectF(-QPointF(bbox.width()/2 , bbox.height()/2), bbox.size()));
            QPointF local_coord = trans.inverted().map(cursor_);
            painter->setPen(ptPen);
            painter->drawPoint(local_coord);
            painter->restore();
        }
    }
}
