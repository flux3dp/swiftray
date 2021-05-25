#include <canvas/transform_box.hpp>
#include <QDebug>

TransformBox::TransformBox() noexcept {
}

void TransformBox::setTarget(Shape *target) {
    QList<Shape *> list;
    list << target;
    setTargets(list);
}

void TransformBox::setTargets(QList<Shape *> &newTargets) {
    clear();

    for (int i = 0; i < newTargets.size(); i++) {
        this->targets << newTargets[i];
        newTargets[i]->selected = true;
    }

    if (targets.size() > 0) {
        qInfo() << "Targets" << targets.size();
    }
}

void TransformBox::clear() {
    for (int i = 0; i < targets.size(); i++) {
        this->targets[i]->selected = false;
    }

    targets.clear();
}

QRectF TransformBox::boundingRect() {
    float top = std::numeric_limits<float>::max();
    float bottom = std::numeric_limits<float>::min();
    float left = std::numeric_limits<float>::max();
    float right = std::numeric_limits<float>::min();

    for (int i = 0; i < targets.size(); i++) {
        QRectF bb = targets[i]->boundingRect();

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
    if (targets.size() == 0) return;

    QRectF bbox = this->boundingRect();
    QPointF center = QPointF(bbox.left() + bbox.width() / 2, bbox.top() + bbox.height() / 2);

    for (int i = 0; i < targets.size(); i++) {
        QPointF newPos = rotatePointAround(targets[i]->pos(), center, rotation);
        targets[i]->x = newPos.x();
        targets[i]->y = newPos.y();
        targets[i]->rot += rotation;
    }
}

void TransformBox::move(QPointF offset) {
    if (targets.size() == 0) return;

    for (int i = 0; i < targets.size(); i++) {
        targets[i]->x += offset.x();
        targets[i]->y += offset.y();
    }
}



QPointF TransformBox::rotatePointAround(QPointF p, QPointF center, double deg) {
    QPointF d(p - center);
    float newTheta = atan2(d.y(), d.x()) + deg * 3.1415926 / 180;
    return center + QPointF(cos(newTheta), sin(newTheta)) * sqrt(d.x() * d.x() + d.y() * d.y());
}
