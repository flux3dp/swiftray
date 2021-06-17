#include <QDebug>
#include <layer.h>
#include <shape/shape.h>
#include <shape/path-shape.h>

const QColor LayerColors[17] = {"#333333", "#3F51B5", "#F44336", "#FFC107", "#8BC34A",
                                "#2196F3", "#009688", "#FF9800", "#CDDC39", "#00BCD4",
                                "#FFEB3B", "#E91E63", "#673AB7", "#03A9F4", "#9C27B0",
                                "#607D8B", "#9E9E9E"};

int layer_color_counter;

Layer::Layer(const QColor &color, const QString &name) {
  color_ = color;
  name_ = name;
  speed_ = 20;
  strength_ = 30;
  repeat_ = 1;
  visible_ = true;
  zstep_ = 0;
  is_diode_ = false;
  target_height_ = 0;
  cache_valid_ = false;
  type_ = Type::Line;
  calcPen();
}

Layer::Layer() :
     Layer(LayerColors[(layer_color_counter++) % 17],
           "Layer 1") {
}

Layer::Layer(int new_layer_id) :
     Layer(LayerColors[(new_layer_id - 1) % 17],
           "Layer " + QString::number(new_layer_id)) {
}


Layer::~Layer() = default;

void Layer::cache() const {
  cache_stack_.begin();
  for (auto &shape : children_) {
    cache_stack_.addShape(shape.get());
  }
  cache_stack_.end();
  cache_valid_ = true;
}

int Layer::paint(QPainter *painter, int counter) const {
  if (!visible_) return 0;
  dash_pen_.setDashOffset(0.3F * counter);

  if (!cache_valid_) cache();
  // Draw shapes
  int painted_objects = 0;
  if (this->type() == Type::Fill || this->type() == Type::FillLine) {
    painter->setBrush(QBrush(color()));
  }
  for (auto &cache : cache_stack_.caches_) {
    switch (cache.type()) {
      case CacheType::SelectedPaths:
        painter->setPen(dash_pen_);
        cache.paint(painter);
        break;
      case CacheType::NonSelectedPaths:
        painter->setPen(solid_pen_);
        cache.paint(painter);
        break;
      case CacheType::Group:
        for (auto &shape : cache.shapes()) {
          painter->setPen(shape->selected() ? dash_pen_ : solid_pen_);
          shape->paint(painter);
        }
        break;
      default:
        cache.paint(painter);
    }
    painted_objects += cache.shapes().size();
  }
  painter->setBrush(Qt::NoBrush);
  return painted_objects;
}

void Layer::addShape(const ShapePtr &shape) {
  shape->setLayer(this);
  children_.push_back(shape);
  cache_valid_ = false;
}

void Layer::removeShape(const ShapePtr &shape) {
  shape->setLayer(nullptr);
  if (!children_.removeOne(shape)) {
    qInfo() << "[Layer] Failed to remove children";
  }
  flushCache();
}

void Layer::removeSelected() {
  children().erase(std::remove_if(children_.begin(), children_.end(),
                                  [](ShapePtr &s) { return s->selected(); }), children_.end());
}

void Layer::calcPen() {
  dash_pen_ = QPen(color_, 2, Qt::DashLine);
  dash_pen_.setDashPattern(QVector<qreal>(10, 3));
  dash_pen_.setCosmetic(true);
  solid_pen_ = QPen(color_, 2, Qt::SolidLine);
  solid_pen_.setCosmetic(true);
}

void Layer::clear() { children_.clear(); }

QColor Layer::color() const { return color_; }

void Layer::setColor(const QColor &color) {
  color_ = color;
  calcPen();
}

QList<ShapePtr> &Layer::children() { return children_; }

LayerPtr Layer::clone() {
  LayerPtr layer = make_shared<Layer>(*this);
  layer->children_.clear();
  for (auto &shape : children_) {
    layer->addShape(shape->clone());
  }
  return layer;
}

int Layer::repeat() const {
  return repeat_;
}

int Layer::speed() const {
  return speed_;
}

int Layer::strength() const {
  return strength_;
}

QString Layer::name() const {
  return name_;
}

void Layer::setHeight(double height) {
  target_height_ = height;
}

void Layer::setName(const QString &name) {
  name_ = name;
}

void Layer::setSpeed(int speed) {
  speed_ = speed;
}

void Layer::setStrength(int strength) {
  strength_ = strength;
}

void Layer::setRepeat(int repeat) {
  repeat_ = repeat;
}

void Layer::setDiode(int diode) {
  is_diode_ = !!diode;
}

void Layer::setZStep(double zstep) {
  zstep_ = zstep;
}

bool Layer::isVisible() const {
  return visible_;
}

void Layer::setVisible(bool visible) {
  visible_ = visible;
}

void Layer::flushCache() {
  cache_valid_ = false;
}

Layer::Type Layer::type() const {
  return type_;
}

void Layer::setType(Layer::Type type) {
  type_ = type;
}
