#include <shape/group_shape.h>

GroupShape::GroupShape() {
}

GroupShape::GroupShape(QList<ShapePtr> &children) {
    children_.append(children);
}

void GroupShape::simplify() {
    for (auto &shape : children_) {
        shape->simplify();
    }
}

void GroupShape::cacheSelectionTestingData() {
    for (auto &shape : children_) {
        shape->cacheSelectionTestingData();
    }
}
bool GroupShape::testHit(QPointF global_coord, qreal tolerance) const {
    QPointF local_coord = transform().inverted().map(global_coord);

    for (auto &shape : children_) {
        if (shape->testHit(local_coord, tolerance)) {
            return true;
        }
    }

    return false;
}
bool GroupShape::testHit(QRectF global_coord_rect) const {
    QRectF local_coord_rect = transform().inverted().mapRect(global_coord_rect);

    for (auto &shape : children_) {
        if (shape->testHit(local_coord_rect)) {
            return true;
        }
    }

    return false;
}
QRectF GroupShape::boundingRect() const {
    float top = std::numeric_limits<float>::max();
    float bottom = std::numeric_limits<float>::min();
    float left = std::numeric_limits<float>::max();
    float right = std::numeric_limits<float>::min();

    for (auto &shape : children_) {
        // TODO: improve bounding box algorithm (draft logic)
        QRectF bb = transform().mapRect(shape->boundingRect());

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

    // TODO: cache this;
    return QRectF(left, top, right - left, bottom - top);
}
void GroupShape::paint(QPainter *painter) const {
    painter->save();
    painter->setTransform(transform(), true);
    painter->drawText(QPointF(), selected ? "Selected" : "Free");

    for (auto &shape : children_) {
        shape->paint(painter);
    }

    painter->restore();
}

ShapePtr GroupShape::clone() const {
    GroupShape *group = new GroupShape();
    group->setTransform(transform());

    for (auto &shape : children_) {
        group->children_.push_back(shape->clone());
    }

    return ShapePtr(group);
}

QList<ShapePtr> &GroupShape::children() {
    return children_;
}

Shape::Type GroupShape::type() const {
    return Shape::Type::Group;
}
