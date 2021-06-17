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

CacheStack::Cache::Cache(CacheType type) : type_(type), is_fill_cached_(false) {}

// Merge all path shape into one joined path
void CacheStack::Cache::merge(const QTransform &base_transform) {
  if (type_ > Type::NonSelectedFilledPaths) return;
  const QRectF &screen_rect = VCanvas::screenRect();
  joined_path_ = QPainterPath();
  for (auto &shape : shapes_) {
    auto *p = (PathShape *) shape;
    QTransform transform = p->transform() * p->tempTransform() * base_transform;
    QPainterPath transformed_path = transform.map(p->path());
    if (transformed_path.intersects(screen_rect)) {
      joined_path_.addPath(transformed_path);
    }
  }
}

const QPixmap &CacheStack::Cache::fillCache(QPainter *painter, QBrush brush) {
  if (!is_fill_cached_) {
    // Get screen information
    QPainterPath screen_rect;
    screen_rect.addRect(VCanvas::screenRect());
    float scale = VCanvas::document().isVolatile() ?
                  VCanvas::document().scale() * 0.5 : VCanvas::document().scale() * 2;
    // Calculate data
    joined_path_ = joined_path_.intersected(screen_rect);
    bbox_ = joined_path_.boundingRect();
    // Draw cache
    cache_pixmap_ = QPixmap(bbox_.size().toSize() * scale);
    cache_pixmap_.fill(QColor::fromRgba64(255, 255, 255, 0));
    QPainter cpaint(&cache_pixmap_);
    cpaint.setRenderHint(QPainter::RenderHint::Antialiasing, !VCanvas::document().isVolatile());
    cpaint.setTransform(
         QTransform()
              .scale(scale, scale)
              .translate(-bbox_.left(), -bbox_.top()),
         false);
    cpaint.fillPath(joined_path_, brush);
    cpaint.end();
    is_fill_cached_ = true;
  }
  painter->drawPixmap(bbox_, cache_pixmap_,
                      QRectF(QPointF(), QSizeF(cache_pixmap_.width(), cache_pixmap_.height())));
  return cache_pixmap_;
}

int CacheStack::paint(QPainter *painter) {
  QElapsedTimer timer;
  timer.start();
  for (auto &cache : caches_) {
    switch (cache.type()) {
      case CacheType::SelectedPaths:
        painter->strokePath(cache.joined_path_, selected_pen_);
        if (force_fill_) cache.fillCache(painter, filling_brush_);
        break;
      case CacheType::SelectedFilledPaths:
        painter->strokePath(cache.joined_path_, selected_pen_);
        cache.fillCache(painter, filling_brush_);
        break;
      case CacheType::NonSelectedPaths:
        painter->strokePath(cache.joined_path_, nonselected_pen_);
        if (force_fill_) cache.fillCache(painter, filling_brush_);
        break;
      case CacheType::NonSelectedFilledPaths:
        cache.fillCache(painter, filling_brush_);
        break;
      case CacheType::Group:
        for (auto &shape : cache.shapes_) {
          ((GroupShape *) shape)->cacheStack().setBrush(filling_brush_);
          ((GroupShape *) shape)->cacheStack().setPen(selected_pen_, nonselected_pen_);
          shape->paint(painter);
        }
        break;
      default:
        for (auto &shape : cache.shapes_) {
          shape->paint(painter);
        }
    }
  }
  painter->setBrush(Qt::NoBrush);
  if (timer.elapsed() > 100) {
    qInfo() << "Cache paint slow" << this << timer.elapsed() << "ms";
  }
  return caches_.size();
}

CacheType CacheStack::Cache::type() const {
  return type_;
}

const QList<Shape *> CacheStack::Cache::shapes() const {
  return shapes_;
}