#include <QDebug>
#include <layer.h>
#include <shape/shape.h>
#include <canvas/vcanvas.h>

const QColor LayerColors[17] = {
     "#333333", "#3F51B5", "#F44336", "#FFC107", "#8BC34A",
     "#2196F3", "#009688", "#FF9800", "#CDDC39", "#00BCD4",
     "#FFEB3B", "#E91E63", "#673AB7", "#03A9F4", "#9C27B0",
     "#607D8B", "#9E9E9E"
};

int layer_color_counter = 0;

// Constructors

Layer::Layer(const QColor &color, const QString &name) {
  color_ = color;
  name_ = name;
  speed_ = 20;
  strength_ = 30;
  repeat_ = 1;
  is_visible_ = true;
  step_height_ = 0;
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
  cache_stack_.setPen(dash_pen_, solid_pen_);
  cache_stack_.setBrush(QBrush(color()));
  cache_stack_.setForceFill(type_ == Type::FillLine || type_ == Type::Fill);

  for (auto &shape : children_) {
    cache_stack_.addShape(shape.get());
  }

  cache_stack_.end();
  cache_valid_ = true;
}

int Layer::paint(QPainter *painter, int counter) const {
  if (!is_visible_) return 0;
  // Update cache
  if (!cache_valid_) cache();
  dash_pen_.setDashOffset(0.3F * counter);
  cache_stack_.setPen(dash_pen_, solid_pen_);
  // Draw shapes
  return cache_stack_.paint(painter);
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

void Layer::calcPen() {
  dash_pen_ = QPen(color_, 2, Qt::DashLine);
  dash_pen_.setDashPattern(QVector<qreal>(10, 3));
  dash_pen_.setCosmetic(true);
  solid_pen_ = QPen(color_, 2, Qt::SolidLine);
  solid_pen_.setCosmetic(true);
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

// Setters

void Layer::setVisible(bool visible) {
  is_visible_ = visible;
}

void Layer::setColor(const QColor &color) {
  auto undo = new PropObjEvent<Layer, QColor, &Layer::color, &Layer::setColor>(this, color_);
  color_ = color;
  calcPen();
  VCanvas::document().addUndoEvent(undo);
}

void Layer::setType(Layer::Type type) {
  auto undo = new PropEvent<Layer, Layer::Type, &Layer::type, &Layer::setType>(this, type_);
  type_ = type;
  flushCache();
  VCanvas::document().addUndoEvent(undo);
}

void Layer::setTargetHeight(double height) {
  auto undo_evt = new PropEvent<Layer, double, &Layer::targetHeight, &Layer::setTargetHeight>(this,
                                                                                              target_height_);
  target_height_ = height;
  VCanvas::document().addUndoEvent(undo_evt);
}

void Layer::setName(const QString &name) {
  auto undo = new PropObjEvent<Layer, QString, &Layer::name, &Layer::setName>(this, name_);
  name_ = name;
  VCanvas::document().addUndoEvent(undo);
}

void Layer::setSpeed(int speed) {
  auto undo = new PropEvent<Layer, int, &Layer::speed, &Layer::setSpeed>(this, speed_);
  speed_ = speed;
  VCanvas::document().addUndoEvent(undo);
}

void Layer::setStrength(int strength) {
  auto undo = new PropEvent<Layer, int, &Layer::strength, &Layer::setStrength>(this, strength_);
  strength_ = strength;
  VCanvas::document().addUndoEvent(undo);
}

void Layer::setRepeat(int repeat) {
  auto undo = new PropEvent<Layer, int, &Layer::repeat, &Layer::setRepeat>(this, repeat_);
  repeat_ = repeat;
  VCanvas::document().addUndoEvent(undo);
}

void Layer::setDiode(bool is_diode) {
  auto undo = new PropEvent<Layer, bool, &Layer::isDiode, &Layer::setDiode>(this, is_diode_);
  is_diode_ = is_diode;
  VCanvas::document().addUndoEvent(undo);
}

void Layer::setStepHeight(double step_height) {
  auto undo = new PropEvent<Layer, double, &Layer::stepHeight, &Layer::setStepHeight>(this, step_height_);
  step_height_ = step_height;
  VCanvas::document().addUndoEvent(undo);
}

// Clone

LayerPtr Layer::clone() {
  LayerPtr layer = make_shared<Layer>(*this);
  layer->children_.clear();
  for (auto &shape : children_) {
    layer->addShape(shape->clone());
  }
  return layer;
}