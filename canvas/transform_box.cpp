#include <canvas/transform_box.hpp>
#include <QDebug>

TransformBox::TransformBox() noexcept {
}

QList<Shape *> *TransformBox::targets() {
    return &this->targets_;
}

void TransformBox::setTarget(Shape *target) {
    QList<Shape *> list;
    list << target;
    setTargets(list);
}

void TransformBox::setTargets(QList<Shape *> &new_targets) {
    clear();

    for (int i = 0; i < new_targets.size(); i++) {
        this->targets_ << new_targets[i];
        new_targets[i]->selected = true;
    }

    if (targets_.size() > 0) {
        qInfo() << "Targets" << targets_.size();
    }
}

void TransformBox::clear() {
    for (int i = 0; i < targets()->size(); i++) {
        targets()->at(i)->simplify();
        targets()->at(i)->selected = false;
    }

    targets()->clear();
}

QRectF TransformBox::boundingRect() {
    float top = std::numeric_limits<float>::max();
    float bottom = std::numeric_limits<float>::min();
    float left = std::numeric_limits<float>::max();
    float right = std::numeric_limits<float>::min();

    for (int i = 0; i < targets()->size(); i++) {
        QRectF bb = targets()->at(i)->boundingRect();

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

    return QRectF(left, top, right - left, bottom - top);
}

bool TransformBox::hasTargets() {
    return targets()->size() > 0;
}

bool TransformBox::containsTarget(Shape *shape) {
    return targets()->contains(shape);
}

void TransformBox::rotate(double rotation) {
    if (targets()->size() == 0) return;

    QRectF bbox = this->boundingRect();
    QTransform groupTransform = QTransform().translate(bbox.center().x(), bbox.center().y())
                                .rotate(rotation)
                                .translate(-bbox.center().x(), -bbox.center().y());
    qInfo() << "Transform =" << groupTransform;

    for (int i = 0; i < targets()->size(); i++) {
        targets()->at(i)->transform(groupTransform);
    }
}

void TransformBox::scale(QPointF scale_center, float scale_x, float scale_y) {
    if (targets()->size() == 0) return;

    QTransform groupTransform = QTransform().translate(scale_center.x(), scale_center.y())
                                .scale(scale_x, scale_y)
                                .translate(-scale_center.x(), -scale_center.y());

    for (int i = 0; i < targets()->size(); i++) {
        targets()->at(i)->transform(groupTransform);
    }
}

void TransformBox::move(QPointF offset) {
    if (targets()->size() == 0) return;

    QTransform trans = QTransform().translate(offset.x(), offset.y());

    for (int i = 0; i < targets_.size(); i++) {
        targets()->at(i)->transform(trans);
    }
}

const QPointF *TransformBox::controlPoints() {
    QRectF bbox = boundingRect();
    control_points_[0] = bbox.topLeft();
    control_points_[1] = (bbox.topLeft() + bbox.topRight()) / 2;
    control_points_[2] = bbox.topRight();
    control_points_[3] = (bbox.topRight() + bbox.bottomRight()) / 2;
    control_points_[4] = bbox.bottomRight();
    control_points_[5] = (bbox.bottomRight() + bbox.bottomLeft()) / 2;
    control_points_[6] = bbox.bottomLeft();
    control_points_[7] = (bbox.bottomLeft() + bbox.topLeft()) / 2;
    return control_points_;
}

TransformBox::ControlPoint TransformBox::testHit(QPointF clickPoint, float tolerance) {
    controlPoints();

    if ((control_points_[0] - clickPoint).manhattanLength() < tolerance) {
        return ControlPoint::NW;
    }

    if ((control_points_[1] - clickPoint).manhattanLength() < tolerance) {
        return ControlPoint::N;
    }

    if ((control_points_[2] - clickPoint).manhattanLength() < tolerance) {
        return ControlPoint::NE;
    }

    if ((control_points_[3] - clickPoint).manhattanLength() < tolerance) {
        return ControlPoint::E;
    }

    if ((control_points_[4] - clickPoint).manhattanLength() < tolerance) {
        return ControlPoint::SE;
    }

    if ((control_points_[5] - clickPoint).manhattanLength() < tolerance) {
        return ControlPoint::S;
    }

    if ((control_points_[6] - clickPoint).manhattanLength() < tolerance) {
        return ControlPoint::SW;
    }

    if ((control_points_[7] - clickPoint).manhattanLength() < tolerance) {
        return ControlPoint::W;
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

bool TransformBox::mousePressEvent(QMouseEvent *e, CanvasData &canvas) {
    pressed_at_ = e->pos();
    QPointF canvas_coord = canvas.getCanvasCoord(e->pos());
    activating_control_ = testHit(canvas_coord, 10 / canvas.scale);
    QPointF d;

    switch (activating_control_) {
    case ControlPoint::ROTATION:
        d  = QPointF(canvas_coord - boundingRect().center());
        transform_rotation = atan2(d.y(), d.x());
        qInfo() << "Transform rotation" << transform_rotation;
        canvas.mode = CanvasData::Mode::TRANSFORMING;
        return true;

    case ControlPoint::NW:
    case ControlPoint::NE:
    case ControlPoint::SW:
    case ControlPoint::SE:
    case ControlPoint::N:
    case ControlPoint::S:
    case ControlPoint::E:
    case ControlPoint::W:
        transform_scaler = QSizeF(boundingRect().size());
        qInfo() << "Transform scaling" << d;
        canvas.mode = CanvasData::Mode::TRANSFORMING;
        return true;

    default:
        qInfo() << "Transform point" << (int)activating_control_;
        return false;
    }
}
bool TransformBox::mouseReleaseEvent(QMouseEvent *e, const CanvasData &canvas) {
    return false;
}
bool TransformBox::mouseMoveEvent(QMouseEvent *e, const CanvasData &canvas) {
    QPointF canvas_coord = canvas.getCanvasCoord(e->pos());

    if (canvas.mode == CanvasData::Mode::SELECTING) {
        move((e->pos() - pressed_at_) / canvas.scale);
        pressed_at_ = e->pos();
        return true;
    }

    if (canvas.mode != CanvasData::Mode::TRANSFORMING) return false;

    QRectF bbox = boundingRect();

    if (activating_control_ == ControlPoint::ROTATION) {
        QPointF d(canvas_coord - bbox.center());
        float new_transform_rotation = atan2(d.y(), d.x());
        qInfo() << "Transform rotation" << new_transform_rotation;
        rotate((new_transform_rotation - transform_rotation) * 180 / 3.1415926);
        transform_rotation = new_transform_rotation;
    }

    QPointF scaling_center;

    switch (activating_control_) {
    case ControlPoint::NW:
        scaling_center = bbox.bottomRight();
        break;

    case ControlPoint::NE:
        scaling_center = bbox.bottomLeft();
        break;

    case ControlPoint::SW:
        scaling_center = bbox.topRight();
        break;

    case ControlPoint::SE:
        scaling_center = bbox.topLeft();
        break;

    case ControlPoint::N:
    case ControlPoint::W:
        scaling_center = bbox.bottomRight();
        break;

    case ControlPoint::S:
    case ControlPoint::E:
        scaling_center = bbox.topLeft();
        break;

    default:
        return false;
    }

    QPointF d(canvas_coord - scaling_center);

    //TODO: improve logic
    if (d.x() == 0 || d.y() == 0) return true;

    qreal sx = abs(d.x() / transform_scaler.width());
    qreal sy = abs(d.y() / transform_scaler.height());

    if (activating_control_ == ControlPoint::N || activating_control_ == ControlPoint::S) {
        sx = 1;
    } else if (activating_control_ == ControlPoint::W || activating_control_ == ControlPoint::E) {
        sy = 1;
    }

    qInfo() << "Transform scaler" << sx << sy;
    scale(scaling_center, sx, sy);
    transform_scaler = QSizeF(boundingRect().size());
    return true;
}

bool TransformBox::hoverEvent(QHoverEvent *e, const CanvasData &canvas, Qt::CursorShape *cursor) {
    ControlPoint cp = testHit(canvas.getCanvasCoord(e->pos()), 10 / canvas.scale);

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
