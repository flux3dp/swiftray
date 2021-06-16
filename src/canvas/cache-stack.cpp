#include <canvas/cache-stack.h>
#include <QDebug>
#include <QList>
#include <shape/path-shape.h>
#include <canvas/vcanvas.h>

void CacheStack::begin(const QTransform &base_transform) {
  base_transform_ = base_transform;
  caches_.clear();
}

void CacheStack::end() {
  for (auto &group : caches_) {
    group.merge(base_transform_);
  }
}

// Categorize the shapes to different cache group
void CacheStack::addShape(Shape *shape) {
  CacheType cache_type;
  switch (shape->type()) {
    case Shape::Type::Path:
    case Shape::Type::Text:
      cache_type = shape->selected() ? CacheType::SelectedPaths : CacheType::NonSelectedPaths;
      break;
    case Shape::Type::Bitmap:
      cache_type = CacheType::Bitmap;
      break;
    case Shape::Type::Group:
      cache_type = CacheType::Group;
      break;
    default:
      Q_ASSERT_X(false, "CacheStack", "Cannot cache unknown typed shapes");
  }

  if (caches_.isEmpty() || caches_.last().type_ != cache_type) {
    caches_.push_back(CacheStack::Cache(cache_type));
  }

  if (shape->type() == Shape::Type::Group) {
    // Flush sub group cache
    shape->flushCache();
  }
  caches_.last().shapes_.push_back(shape);
}

CacheStack::Cache::Cache(CacheType type) : type_(type) {}

void CacheStack::Cache::merge(const QTransform &base_transform) {
  if (type_ != Type::SelectedPaths && type_ != Type::NonSelectedPaths) return;
  const QRectF &screen_rect = VCanvas::screenRect();
  for (auto &shape : shapes_) {
    auto *p = (PathShape *) shape;
    QTransform transform = p->transform() * p->tempTransform() * base_transform;
    QPainterPath transformed_path = transform.map(p->path());
    if (transformed_path.intersects(screen_rect)) {
      joined_path_.addPath(transformed_path);
    }
  }
}

void CacheStack::Cache::paint(QPainter *painter) {
  if (type_ == Type::SelectedPaths || type_ == Type::NonSelectedPaths) {
    painter->drawPath(joined_path_);
  } else {
    for (auto &shape : shapes_) {
      shape->paint(painter);
    }
  }
}

CacheType CacheStack::Cache::type() const {
  return type_;
}

const QList<Shape *> CacheStack::Cache::shapes() const {
  return shapes_;
}