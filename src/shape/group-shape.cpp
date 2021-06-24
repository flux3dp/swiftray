#include <QDebug>
#include <layer.h>
#include <shape/group-shape.h>

GroupShape::GroupShape() :
     Shape(),
     cache_(make_unique<CacheStack>(this)) {}

GroupShape::GroupShape(QList<ShapePtr> &children) :
     Shape(),
     cache_(make_unique<CacheStack>(this)) {
  children_ = children;
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
  this->bbox_ = transform().mapRect(local_bbox);
  this->rotated_bbox_ = transform().map(QPolygonF(local_bbox));
  // TODO (Move group's refresh cache outside of bounding rect)
  cache();
}

void GroupShape::cache() const {
  if (!hasLayer()) return;
  qInfo() << "Group caching" << this << " called";
  // TODO: (This breaks if the group is inside another group! Consider using global transform)
  // note that temp transform is not considered here
  cache_->update();
}

void GroupShape::paint(QPainter *painter) const {
  boundingRect();

  cache_->paint(painter);
}

ShapePtr GroupShape::clone() const {
  GroupShape *group = new GroupShape();
  group->setTransform(transform());
  group->setRotation(rotation());

  for (auto &shape : children_) {
    ShapePtr new_shape = shape->clone();
    new_shape->setParent(group);
    group->children_.push_back(new_shape);
  }

  return ShapePtr(group);
}

const QList<ShapePtr> &GroupShape::children() const { return children_; }

Shape::Type GroupShape::type() const { return Shape::Type::Group; }

CacheStack &GroupShape::cacheStack() const {
  return *cache_.get();
}