#include <canvas/controls/transform_box.h>
#include <QDebug>
#include <QObject>

TransformBox::TransformBox(Scene &scene) noexcept : CanvasControl(scene) {
    connect(this, SIGNAL(transformChanged()), this, SLOT(updateBoundingRect()));
    connect(&((QObject &)scene_), SIGNAL(selectionsChanged()), this, SLOT(updateSelections()));
}

QList<ShapePtr> &TransformBox::selections() {
    return selections_;
}

void TransformBox::updateSelections() {
    for (int i = 0; i < selections().size() ; i ++) {
        // Recalculate the click testing points
        selections().at(i)->simplify();
    }

    selections().clear();
    selections().append(scene_.selections());
    updateBoundingRect();
}

void TransformBox::updateBoundingRect() {
    float top = std::numeric_limits<float>::max();
    float bottom = std::numeric_limits<float>::min();
    float left = std::numeric_limits<float>::max();
    float right = std::numeric_limits<float>::min();

    for (int i = 0; i < selections().size(); i++) {
        QRectF bb = selections().at(i)->boundingRect();

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
}

QRectF TransformBox::boundingRect() {
    return bounding_rect_;
}

void TransformBox::rotate(double rotation) {
    if (selections().size() == 0) return;

    QTransform groupTransform = QTransform().translate(action_center_.x(), action_center_.y())
                                .rotate(rotation)
                                .translate(-action_center_.x(), -action_center_.y());

    for (int i = 0; i < selections().size(); i++) {
        selections().at(i)->applyTransform(groupTransform);
    }

    emit transformChanged();
}

void TransformBox::scale(QPointF scale_center, float scale_x, float scale_y) {
    if (selections().size() == 0) return;

    QTransform groupTransform = QTransform().translate(scale_center.x(), scale_center.y())
                                .scale(scale_x, scale_y)
                                .translate(-scale_center.x(), -scale_center.y());

    for (int i = 0; i < selections().size(); i++) {
        selections().at(i)->applyTransform(groupTransform);
    }

    emit transformChanged();
}

void TransformBox::move(QPointF offset) {
    if (selections().size() == 0) return;

    QTransform trans = QTransform().translate(offset.x(), offset.y());

    for (int i = 0; i < selections().size(); i++) {
        selections()[i]->applyTransform(trans);
    }

    emit transformChanged();
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
    pressed_at_ = e->pos();
    QPointF canvas_coord = scene().getCanvasCoord(e->pos());
    QRectF bbox = boundingRect();
    QPointF d;
    activating_control_ = testHit(canvas_coord, 10 / scene().scale());

    switch (activating_control_) {
    case ControlPoint::ROTATION:
        action_center_ = bbox.center();
        d  = QPointF(canvas_coord - action_center_);
        transform_rotation = atan2(d.y(), d.x());
        init_rotation_rect_ = boundingRect().translated(-action_center_);
        cumulated_rotation_ = 0;
        qInfo() << "Transform rotation" << transform_rotation;
        scene().stackStep();
        scene().setMode(Scene::Mode::TRANSFORMING);
        return true;

    case ControlPoint::NW:
        action_center_ = bbox.bottomRight();
        break;

    case ControlPoint::NE:
        action_center_ = bbox.bottomLeft();
        break;

    case ControlPoint::SW:
        action_center_ = bbox.topRight();
        break;

    case ControlPoint::SE:
        action_center_ = bbox.topLeft();
        break;

    case ControlPoint::N:
    case ControlPoint::W:
        action_center_ = bbox.bottomRight();
        break;

    case ControlPoint::S:
    case ControlPoint::E:
        action_center_ = bbox.topLeft();
        break;

    default:
        qInfo() << "Transform point" << (int)activating_control_;
        return false;
    }

    transform_scaler = QSizeF(boundingRect().size());
    flipped_x = flipped_y = false;
    qInfo() << "Transform scaling" << d;
    scene().stackStep();
    scene().setMode(Scene::Mode::TRANSFORMING);
    return true;
}

bool TransformBox::mouseReleaseEvent(QMouseEvent *e) {
    if (activating_control_ != ControlPoint::NONE) {
        activating_control_ = ControlPoint::NONE;
        flipped_x = flipped_y = false;
    } else if (scene().mode() == Scene::Mode::MOVING) {
        scene().setMode(Scene::Mode::SELECTING);
        scene().stackStep();
    } else {
        return false;
    }

    scene().stackStep();
    return true;
}

bool TransformBox::mouseMoveEvent(QMouseEvent *e) {
    if (scene().selections().size() == 0) return false;

    QPointF canvas_coord = scene().getCanvasCoord(e->pos());

    if (scene().mode() == Scene::Mode::MOVING) {
        move((e->pos() - pressed_at_) / scene().scale());
        pressed_at_ = e->pos();
        return true;
    }

    if (scene().mode() != Scene::Mode::TRANSFORMING) return false;

    if (activating_control_ == ControlPoint::ROTATION) {
        QPointF d(canvas_coord - action_center_);
        float new_transform_rotation = atan2(d.y(), d.x());
        qInfo() << "Transform rotation" << new_transform_rotation;
        cumulated_rotation_ += (new_transform_rotation - transform_rotation) * 180 / 3.1415926;
        rotate((new_transform_rotation - transform_rotation) * 180 / 3.1415926);
        transform_rotation = atan2(d.y(), d.x());
        return true;
    }

    // Scaling
    //TODO: improve logic
    QPointF d(canvas_coord - action_center_);

    if (d.x() == 0 || d.y() == 0) return true;

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

    qreal sx = d.x() / transform_scaler.width();
    qreal sy = d.y() / transform_scaler.height();

    if (activating_control_ == ControlPoint::N || activating_control_ == ControlPoint::S) {
        sx = 1;
    } else if (activating_control_ == ControlPoint::W || activating_control_ == ControlPoint::E) {
        sy = 1;
    }

    if (d.x() > 0 && flipped_x) {
        flipped_x = false;
        sx = -sx;
    }

    if (d.y() > 0 && flipped_y) {
        flipped_y = false;
        sy = -sy;
    }

    if (flipped_x) sx = abs(sx);

    if (flipped_y) sy = abs(sy);

    if (d.x() < 0) flipped_x = true;

    if (d.y() < 0) flipped_y = true;

    qInfo() << "Transforming scaler" << sx << sy << d.x() << d.y() << flipped_x << flipped_y;
    scale(action_center_, sx, sy);
    transform_scaler = QSizeF(boundingRect().size());
    return true;
}

bool TransformBox::hoverEvent(QHoverEvent *e, Qt::CursorShape *cursor) {
    ControlPoint cp = testHit(scene().getCanvasCoord(e->pos()), 10 / scene().scale());

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
    QColor sky_blue = QColor::fromRgb(0x00, 0x99, 0xCC, 255);
    QPen bluePen = QPen(QBrush(sky_blue), 0, Qt::DashLine);
    QPen ptPen(sky_blue, 10 / scene().scale(), Qt::PenStyle::SolidLine, Qt::RoundCap);

    if (selections().size() > 0) {
        if (activating_control_ == ControlPoint::ROTATION) {
            painter->save();
            painter->translate(action_center_);
            painter->setPen(bluePen);
            painter->rotate(cumulated_rotation_);
            painter->drawRect(init_rotation_rect_);
            painter->restore();
        } else {
            painter->setPen(bluePen);
            painter->drawPolyline(controlPoints(), 8);
            painter->drawLine(controlPoints()[7], controlPoints()[0]);
            painter->setPen(ptPen);
            painter->drawPoints(controlPoints(), 8);
            painter->drawPoint(boundingRect().center());
        }
    }
}
