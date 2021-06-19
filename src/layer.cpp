#include <QDebug>
#include <layer.h>
#include <shape/shape.h>
#include <canvas/canvas.h>

// TODO (remove this and make this logic to svgpp parser)
int layer_color_counter = 0;

// Constructors
Layer::Layer(Document *doc, const QColor &color, const QString &name) :
     document_(doc),
     color_(color),
     name_(name),
     speed_(20),
     strength_(30),
     repeat_(1),
     is_visible_(true),
     step_height_(0),
     is_diode_(false),
     target_height_(0),
     cache_(make_unique<CacheStack>(this)),
     cache_valid_(false),
     type_(Type::Line) {}

Layer::Layer(Document *doc, int layer_id) :
     Layer(doc,
           Layer::DefaultColors.at((layer_id - 1) % 17),
           "Layer " + QString::number(layer_id)) {}

Layer::Layer(Document *doc) :
     Layer(doc, 1) {}


Layer::Layer() :
     Layer(nullptr, 1) {}

Layer::~Layer() = default;

int Layer::paint(QPainter *painter) const {
  if (!is_visible_) return 0;
  // Update cache if it's not valid
  if (!cache_valid_) {
    cache_->update();
    cache_valid_ = true;
  }
  // Draw shapes
  return cache_->paint(painter);
}

void Layer::addShape(const ShapePtr &shape) {
  shape->setLayer(this);
  children_.push_back(shape);
  cache_valid_ = false;
}

void Layer::removeShape(const ShapePtr &shape) {
  if (!children_.removeOne(shape)) {
    qInfo() << "[Layer] Failed to remove children";
  }
  flushCache();
}

// Getters

const QColor &Layer::color() const { return color_; }

QList<ShapePtr> &Layer::children() { return children_; }

int Layer::repeat() const {
  return repeat_;
}

int Layer::speed() const {
  return speed_;
}

int Layer::strength() const {
  return strength_;
}

const QString &Layer::name() const {
  return name_;
}

bool Layer::isVisible() const {
  return is_visible_;
}

bool Layer::isDiode() const {
  return is_diode_;
}

void Layer::flushCache() {
  cache_valid_ = false;
}

double Layer::stepHeight() const { return step_height_; }

double Layer::targetHeight() const { return target_height_; }

Layer::Type Layer::type() const { return type_; }

Document &Layer::document() {
  Q_ASSERT_X(document_ != nullptr,
             "Layer",
             "This layer does not belong to any document."
             "The constructing process might be wrong.");
  return *document_;
}

// Setters

void Layer::setVisible(bool visible) {
  is_visible_ = visible;
}

void Layer::setColor(const QColor &color) {
  color_ = color;
  flushCache();
}

void Layer::setType(Layer::Type type) {
  type_ = type;
  flushCache();
}

void Layer::setTargetHeight(double height) {
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

void Layer::setDiode(bool is_diode) {
  is_diode_ = is_diode;
}

void Layer::setStepHeight(double step_height) {
  step_height_ = step_height;
}

void Layer::setDocument(Document *doc) {
  document_ = doc;
}

// Clone

LayerPtr Layer::clone() {
  LayerPtr new_layer = make_shared<Layer>(document_, color(), name());
  new_layer->setSpeed(speed());
  new_layer->setStrength(strength());
  new_layer->setRepeat(repeat());
  new_layer->setVisible(isVisible());
  new_layer->setStepHeight(stepHeight());
  new_layer->setDiode(isDiode());
  new_layer->setTargetHeight(targetHeight());
  new_layer->setType(type());
  for (auto &shape : children_) {
    new_layer->addShape(shape->clone());
  }
  return new_layer;
}

