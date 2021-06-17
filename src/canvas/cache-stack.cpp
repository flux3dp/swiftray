#include <canvas/cache-stack.h>
#include <QDebug>
#include <QList>
#include <shape/path-shape.h>
#include <shape/group-shape.h>
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
      if (((PathShape *) shape)->isFilled()) {
        cache_type = shape->selected() ? CacheType::SelectedFilledPaths : CacheType::NonSelectedFilledPaths;
      } else {
        cache_type = shape->selected() ? CacheType::SelectedPaths : CacheType::NonSelectedPaths;
      }
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
  if (type_ > Type::NonSelectedFilledPaths) return;
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

int CacheStack::paint(QPainter *painter) {
  for (auto &cache : caches_) {
    switch (cache.type()) {
      case CacheType::SelectedPaths:
        painter->setBrush(force_fill_ ? filling_brush_ : Qt::NoBrush);
        painter->setPen(selected_pen_);
        painter->drawPath(cache.joined_path_);
        break;
      case CacheType::SelectedFilledPaths:
        painter->setBrush(filling_brush_);
        painter->setPen(selected_pen_);
        painter->drawPath(cache.joined_path_);
        break;
      case CacheType::NonSelectedPaths:
        painter->setBrush(force_fill_ ? filling_brush_ : Qt::NoBrush);
        painter->setPen(nonselected_pen_);
        painter->drawPath(cache.joined_path_);
        break;
      case CacheType::NonSelectedFilledPaths:
        painter->setBrush(filling_brush_);
        painter->setPen(nonselected_pen_);
        painter->drawPath(cache.joined_path_);
        break;
      case CacheType::Group:
        for (auto &shape : cache.shapes_) {
          shape->flushCache();
          ((GroupShape *) shape)->cacheStack().setBrush(filling_brush_);
          ((GroupShape *) shape)->cacheStack().setPen(selected_pen_, nonselected_pen_);
          shape->paint(painter);
        }
      default:
        for (auto &shape : cache.shapes_) {
          shape->paint(painter);
        }
    }
  }
  painter->setBrush(Qt::NoBrush);
  return caches_.size();
}

CacheType CacheStack::Cache::type() const {
  return type_;
}

const QList<Shape *> CacheStack::Cache::shapes() const {
  return shapes_;
}