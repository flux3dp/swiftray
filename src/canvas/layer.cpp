#include <QDebug>
#include <canvas/layer.h>
#include <shape/shape.h>
#include <shape/path_shape.h>

const QColor LayerColors[17] = {"#333333", "#3F51B5", "#F44336", "#FFC107", "#8BC34A",
                                "#2196F3", "#009688", "#FF9800", "#CDDC39", "#00BCD4",
                                "#FFEB3B", "#E91E63", "#673AB7", "#03A9F4", "#9C27B0",
                                "#607D8B", "#9E9E9E"};

int layer_color_counter;

Layer::Layer(QColor color, QString name) {
  color_ = color;
  name_ = name;
  speed_ = 20;
  strength_ = 30;
  repeat_ = 1;
  visible_ = true;
  cache_valid_ = false;
}

Layer::Layer() :
     Layer(LayerColors[(layer_color_counter++) % 17],
           "Layer 1") {
}

Layer::Layer(int new_layer_id) :
     Layer(LayerColors[(new_layer_id - 1) % 17],
           "Layer " + QString::number(new_layer_id)) {
}

void Layer::cache() const {
  selected_path_.clear();
  non_selected_path_.clear();
  for (auto &shape : children_) {
    if (shape->type() == Shape::Type::Path) {
      PathShape *p = (PathShape *) shape.get();
      if (shape->selected()) {
        selected_path_.addPath((shape->transform() * shape->tempTransform()).map(p->path()));
      } else {
        non_selected_path_.addPath(shape->transform().map(p->path()));
      }
    }
  }
  cache_valid_ = true;
}

void Layer::paint(QPainter *painter, int counter) const {
  if (!visible_) return;
  QPen dash_pen = QPen(color_, 2, Qt::DashLine);
  dash_pen.setDashPattern(QVector<qreal>(10, 3));
  dash_pen.setCosmetic(true);
  dash_pen.setDashOffset(0.3F * counter);
  QPen solid_pen = QPen(color_, 2, Qt::SolidLine);
  solid_pen.setCosmetic(true);

  bool selected_flag = false;
  painter->setPen(solid_pen);
  // Draw shapes
  if (!cache_valid_) {
    cache();
  }
  //TODO(Make multiple caches between non-path item)
  for (auto &shape : children_) {
    if (shape->type() != Shape::Type::Path) {
      shape->paint(painter);
    }
  }
  painter->setPen(dash_pen);
  painter->drawPath(selected_path_);
  painter->setPen(solid_pen);
  painter->drawPath(non_selected_path_);
}

void Layer::addShape(ShapePtr shape) {
  shape->setParent(this);
  children_.push_back(shape);
  cache_valid_ = false;
}

void Layer::removeShape(ShapePtr shape) {
  shape->setParent(nullptr);
  children_.removeOne(shape);
}

void Layer::clear() { children_.clear(); }

QColor Layer::color() const { return color_; }

void Layer::setColor(QColor color) { color_ = color; }

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
  height_ = height;
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
  diode_ = !!diode;
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

void Layer::invalidCache() {
  cache_valid_ = false;
}