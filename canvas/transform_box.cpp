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

void TransformBox::rotate(double rotation) {
    if (targets_.size() == 0) return;

    QRectF bbox = this->boundingRect();
    QPointF center = QPointF(bbox.left() + bbox.width() / 2, bbox.top() + bbox.height() / 2);

    for (int i = 0; i < targets()->size(); i++) {
        QPointF newPos = rotatePointAround(targets()->at(i)->pos(), center, rotation);
        targets()->at(i)->x = newPos.x();
        targets()->at(i)->y = newPos.y();
        targets()->at(i)->rot += rotation;
    }
}

void TransformBox::move(QPointF offset) {
    if (targets()->size() == 0) return;

    for (int i = 0; i < targets_.size(); i++) {
        targets()->at(i)->x += offset.x();
        targets()->at(i)->y += offset.y();
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


QPointF TransformBox::rotatePointAround(QPointF p, QPointF center, double deg) {
    QPointF d(p - center);
    float newTheta = atan2(d.y(), d.x()) + deg * 3.1415926 / 180;
    return center + QPointF(cos(newTheta), sin(newTheta)) * sqrt(d.x() * d.x() + d.y() * d.y());
}
