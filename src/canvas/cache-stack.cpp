#include <canvas/cache-stack.h>
#include <QDebug>
#include <QList>

#include <shape/path-shape.h>
#include <shape/group-shape.h>
#include <canvas/canvas.h>

CacheStack::CacheStack(GroupShape *group) :
     group_(group),
     layer_(nullptr),
     type_(Type::Group) {}

CacheStack::CacheStack(Layer *layer) :
     layer_(layer),
     group_(nullptr),
     type_(Type::Layer) {}

void CacheStack::update() {
  caches_.clear();

  // Load children
  if (isLayer()) {
    for (auto &shape: layer_->children()) {
      addShape(shape.get());
    }
  } else {
    global_transform_ = group_->globalTransform();
    for (auto &shape: group_->children()) {
      addShape(shape.get());
    }
  }
  // Merge paths
  for (auto &cache : caches_) {
    cache.merge(global_transform_);
  }
}

// Categorize the shapes to different cache group
void CacheStack::addShape(Shape *shape) {
  CacheType cache_type;
  switch (shape->type()) {
    case Shape::Type::Path:
    case Shape::Type::Text:
      if (isLayer()) {
        if (((PathShape *) shape)->isFilled()) {
          cache_type = shape->selected() ? CacheType::SelectedFilledPaths : CacheType::NonSelectedFilledPaths;
        } else {
          cache_type = shape->selected() ? CacheType::SelectedPaths : CacheType::NonSelectedPaths;
        }
      } else {
        cache_type = ((PathShape *) shape)->isFilled() ? CacheType::NonSelectedFilledPaths
                                                       : CacheType::NonSelectedPaths;
      }
      break;
    case Shape::Type::Bitmap:
      cache_type = CacheType::Bitmap;
      break;
    case Shape::Type::Group:
      cache_type = CacheType::Group;
      ((GroupShape *) shape)->cacheStack().update(); // If the shape is a group, we need to flush it, but do not propagate event back to the top
      break;
    default:
      Q_ASSERT_X(false, "CacheStack", "Cannot cache unknown typed shapes");
  }

  if (caches_.isEmpty() || caches_.last().type() != cache_type) {
    caches_.push_back(CacheStack::Cache(this, cache_type));
  }

  caches_.last().addShape(shape);
}

// CacheFragment constructor
CacheStack::Cache::Cache(CacheStack *stack,
                         CacheType type) :
     stack_(stack),
     type_(type),
     is_fill_cached_(false) {}

void CacheStack::Cache::addShape(Shape *shape) {
  shapes_.push_back(shape);
}

// CacheFragment merging all path shape into one joined path
void CacheStack::Cache::merge(const QTransform &global_transform) {
  if (type_ > Type::NonSelectedFilledPaths) return;
  const QRectF &screen_rect = stack_->document().screenRect();
  joined_path_ = QPainterPath();
  for (auto &shape : shapes_) {
    auto *p = (PathShape *) shape;
    QTransform transform = p->transform() * p->tempTransform() * global_transform;
    QPainterPath transformed_path = transform.map(p->path());
    if (transformed_path.intersects(screen_rect)) {
      joined_path_.addPath(transformed_path);
    }
  }
}

void CacheStack::Cache::cacheFill() {
  // Get screen information
  QPainterPath screen_rect;
  screen_rect.addRect(stack_->document().screenRect());
  float scale = stack_->document().scale() * (stack_->canvas().isVolatile() ? 1 : 2);
  // Clip with screen_rect
  joined_path_ = joined_path_.intersected(screen_rect);
  bbox_ = joined_path_.boundingRect();
  // Draw filled path to pixmap cache
  cache_pixmap_ = QPixmap(bbox_.size().toSize() * scale);
  cache_pixmap_.fill(QColor::fromRgba64(255, 255, 255, 0));
  QPainter cpaint(&cache_pixmap_);
  cpaint.setRenderHint(QPainter::RenderHint::Antialiasing, !stack_->canvas().isVolatile());
  cpaint.setTransform(
       QTransform()
            .scale(scale, scale)
            .translate(-bbox_.left(), -bbox_.top()),
       false);
  cpaint.fillPath(joined_path_, stack_->color());
  cpaint.end();
}

void CacheStack::Cache::stroke(QPainter *painter, const QPen &pen) {
  painter->strokePath(joined_path_, pen);
}

void CacheStack::Cache::fill(QPainter *painter, const QPen &pen) {
  if (!is_fill_cached_) {
    cacheFill();
    is_fill_cached_ = true;
  }
  painter->drawPixmap(bbox_, cache_pixmap_,
                      QRectF(QPointF(), QSizeF(cache_pixmap_.width(), cache_pixmap_.height())));
}


// Primary logic for painting shapes
int CacheStack::paint(QPainter *painter) {
  QElapsedTimer timer;
  timer.start();

  Layer::Type layer_type = isGroup() ? group_->layer()->type() : layer_->type();
  bool always_fill = layer_type == Layer::Type::Fill || layer_type == Layer::Type::FillLine;
  bool always_select = isGroup() ? group_->isParentSelected() : false;

  QPen dash_pen(color(), 2, Qt::DashLine);
  if (isGroup()) {
    dash_pen.setDashPattern(QVector<qreal>({18, 3, 9, 3}));
  }
  dash_pen.setDashOffset(document().framesCount());
  dash_pen.setCosmetic(true);
  QPen solid_pen(color(), 2, Qt::SolidLine);
  solid_pen.setCosmetic(true);
  QBrush brush(color());


  for (auto &cache : caches_) {
    switch (cache.type()) {
      case CacheType::SelectedPaths:
        cache.stroke(painter, dash_pen);
        if (always_fill) cache.fill(painter, dash_pen);
        break;
      case CacheType::SelectedFilledPaths:
        cache.stroke(painter, dash_pen);
        cache.fill(painter, dash_pen);
        break;
      case CacheType::NonSelectedPaths:
        cache.stroke(painter, always_select ? dash_pen : solid_pen);
        if (always_fill) cache.fill(painter, dash_pen);
        break;
      case CacheType::NonSelectedFilledPaths:
        cache.stroke(painter, always_select ? dash_pen : solid_pen);
        cache.fill(painter, dash_pen);
        break;
      default:
        for (auto &shape : cache.shapes()) {
          bool use_dash = always_select || (!isGroup() && shape->selected());
          painter->setPen(use_dash ? dash_pen : Qt::NoPen);
          shape->paint(painter);
          painter->setPen(Qt::NoPen);
        }
    }
  }
  painter->setBrush(Qt::NoBrush);
  if (timer.elapsed() > 500) {
    qDebug() << "[Painting] Slow Rendering (" << this << timer.elapsed() << "ms)";
  }
  return caches_.size();
}

CacheType CacheStack::Cache::type() const {
  return type_;
}

const QList<Shape *> &CacheStack::Cache::shapes() const {
  return shapes_;
}

bool CacheStack::isGroup() const {
  return type_ == Type::Group;
}

bool CacheStack::isLayer() const {
  return type_ == Type::Layer;
}

const Canvas &CacheStack::canvas() const { return *document().canvas(); }

const Document &CacheStack::document() const { return isGroup() ? group_->layer()->document() : layer_->document(); }

const QColor &CacheStack::color() const { return isGroup() ? group_->layer()->color() : layer_->color(); }