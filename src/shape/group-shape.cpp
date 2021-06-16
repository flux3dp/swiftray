#include <QDebug>
#include <layer.h>
#include <shape/group-shape.h>

GroupShape::GroupShape() : Shape() {}

GroupShape::GroupShape(QList<ShapePtr> &children) : Shape() {
  children_.append(children);
  flushCache();
}

bool GroupShape::hitTest(QPointF global_coord, qreal tolerance) const {
  QPointF local_coord = transform().inverted().map(global_coord);

  for (auto &shape : children_) {
    if (shape->hitTest(local_coord, tolerance)) {
      return true;
    }
  }

  return false;
}

bool GroupShape::hitTest(QRectF global_coord_rect) const {
  QRectF local_coord_rect = transform().inverted().mapRect(global_coord_rect);

  for (auto &shape : children_) {
    if (shape->hitTest(local_coord_rect)) {
      return true;
    }
  }

  return false;
}

void GroupShape::calcBoundingBox() const {
  float top = std::numeric_limits<float>::max();
  float bottom = std::numeric_limits<float>::min();
  float left = std::numeric_limits<float>::max();
  float right = std::numeric_limits<float>::min();

  for (auto &shape : children_) {
    // TODO: improve bounding box algorithm (draft logic)
    QRectF bb = shape->boundingRect();

    if (bb.left() < left)
      left = bb.left();
    if (bb.top() < top)
      top = bb.top();
    if (bb.right() > right)
      right = bb.right();
    if (bb.bottom() > bottom)
      bottom = bb.bottom();
  }

  QRectF local_bbox = QRectF(left, top, right - left, bottom - top);
  qInfo() << "Group local bbox" << local_bbox;
  this->bbox_ = transform().mapRect(local_bbox);
  this->rotated_bbox_ = transform().map(QPolygonF(local_bbox));
  cache();
}

void GroupShape::cache() const {
  if (!hasLayer()) return;
  cache_stack_.begin(transform_ * temp_transform_);
  for (auto &shape : children_) {
    cache_stack_.addShape(shape.get());
  }
  cache_stack_.end();
}

void GroupShape::paint(QPainter *painter) const {
  boundingRect();

  for (auto &cache : cache_stack_.caches_) {
    cache.paint(painter);
  }
}

ShapePtr GroupShape::clone() const {
  GroupShape *group = new GroupShape();
  group->setTransform(transform());
  group->setRotation(rotation());

  for (auto &shape : children_) {
    group->children_.push_back(shape->clone());
  }

  return ShapePtr(group);
}

const QList<ShapePtr> &GroupShape::children() const { return children_; }

Shape::Type GroupShape::type() const { return Shape::Type::Group; }
