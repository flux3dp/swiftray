#include <QDebug>
#include <layer.h>
#include <shape/shape.h>
#include <canvas/canvas.h>
#include <QObject>
#include <QString>
#include <QList>

// Layer colors
static QList<QString> layer_colors = {
     "#333333", "#3F51B5", "#F44336", "#FFC107", "#8BC34A",
     "#2196F3", "#009688", "#FF9800", "#CDDC39", "#00BCD4",
     "#FFEB3B", "#E91E63", "#673AB7", "#03A9F4", "#9C27B0",
     "#607D8B", "#9E9E9E"
};

// Constructors
Layer::Layer(Document *doc, const QColor &color, const QString &name) :
     document_(doc),
     color_(color),
     name_(name),
     speed_(20),
     power_(30),
     repeat_(1),
     x_backlash_(0),
     parameter_index_(-1),
     is_locked_(false),
     is_visible_(true),
     step_height_(0),
     use_diode_(false),
     target_height_(0),
     type_(Type::Line) {}

Layer::Layer(Document *doc, int layer_counter) :
     Layer(doc,
           layer_colors[(layer_counter-1) % layer_colors.count()],
           QObject::tr("Layer") + " " + QString::number(layer_counter)) {}

Layer::Layer() :
     Layer(nullptr, Qt::black, QObject::tr("Layer")) {}

Layer::~Layer() = default;

void Layer::paintUnselected(QPainter *painter, double line_width) {
  if (!is_visible_) return;
  // Draw shapes
  QPen layer_stroke_pen(color_, line_width, Qt::SolidLine);
  layer_stroke_pen.setCosmetic(true);
  for (ShapePtr &shape : children_) {
    if(shape->selected()) continue;
    painter->setPen(layer_stroke_pen);
    shape.get()->paint(painter);
  }
}

void Layer::addShape(const ShapePtr &shape) {
  shape->setLayer(this);
  children_mutex_.lock();
  if(type_ == Layer::Type::Line) shape->setFilled(false);
  else shape->setFilled(true);
  children_.push_back(shape);
  children_mutex_.unlock();
}

void Layer::removeShape(const ShapePtr &shape) {
  children_mutex_.lock();
  if (!children_.removeOne(shape)) {
    qInfo() << "[Layer] Failed to remove children";
  }
  children_mutex_.unlock();
}

// Getters

const QColor &Layer::color() const { return color_; }

QList<ShapePtr> &Layer::children() { return children_; }

int Layer::repeat() const {
  return repeat_;
}

double Layer::speed() const {
  return speed_;
}

double Layer::power() const {
  return power_;
}

double Layer::xBacklash() const {
  return x_backlash_;
}

int Layer::parameterIndex() const {
  return parameter_index_;
}

const QString &Layer::name() const {
  return name_;
}

bool Layer::isLocked() const {
    return is_locked_;
}

bool Layer::isVisible() const {
  return is_visible_;
}

bool Layer::isUseDiode() const {
  return use_diode_;
}

double Layer::stepHeight() const { return step_height_; }

double Layer::targetHeight() const { return target_height_; }

int Layer::module() const { return module_; }

float Layer::focus() const { return focus_; }

float Layer::focusStep() const { return focus_step_; }

float Layer::printingStrength() const { return printing_strength_; }

double Layer::printingSpeed() const { return printing_speed_; }

int Layer::uv() const { return uv_; }

int Layer::halftone() const { return halftone_; }

int Layer::multipass() const { return multipass_; }

int Layer::ink() const { return ink_; }

double Layer::minPower() const { return min_power_; }

Layer::Type Layer::type() const { return type_; }

int Layer::frequency() const { return frequency_; }

int Layer::pulseWidth() const { return pulse_width_; }

double Layer::fillInterval() const { return fill_interval_; }

double Layer::fillAngle() const { return fill_angle_; }

bool Layer::fillBidirectional() const { return fill_bidirectional_; }

bool Layer::fillHatch() const { return fill_hatch_; }

Document &Layer::document() {
  Q_ASSERT_X(document_ != nullptr,
             "Layer",
             "This layer does not belong to any document."
             "The constructing process might be wrong.");
  return *document_;
}

// Setters

void Layer::setLocked(bool isLocked) {
    is_locked_ = isLocked;
}

void Layer::setVisible(bool visible) {
  is_visible_ = visible;
}

void Layer::setColor(const QColor &color) {
  color_ = color;
}

void Layer::setType(Layer::Type type) {
  // TODO: Whether setFilled of all shapes in this layer?
  type_ = type;
  for (ShapePtr &shape : children_) {
    if(type == Layer::Type::Line) shape->setFilled(false);
    else shape->setFilled(true);
  }
}

void Layer::setName(const QString &name) {
  name_ = name;
}

void Layer::setParameterIndex(int parameter_index) {
  parameter_index_ = parameter_index;
}

void Layer::setDocument(Document *doc) {
  document_ = doc;
}

// UI Adjustable Parameters in Swiftray
void Layer::setParameters(const LayerParameters &params) {
  // use attr instead of setter
  this->speed_ = params.speed;
  this->power_ = params.strength;
  this->repeat_ = params.repeat;
  this->x_backlash_ = params.backlash;
  this->frequency_ = params.frequency;
  this->pulse_width_ = params.pulse_width;
  this->fill_interval_ = params.fill_interval;
  this->fill_angle_ = params.fill_angle;
}

// Adjustable parameters in Beam Studio (including Ador's printing module)
void Layer::setParameters(const MySVG::BeamLayerConfig &config) {
  this->is_visible_ = config.visible;
  this->speed_ = config.speed;
  this->power_ = config.power;
  this->color_ = config.color;
  this->module_ = config.module;
  this->repeat_ = config.repeat;
  this->target_height_ = config.height;
  this->step_height_ = config.z_step;
  this->use_diode_ = config.diode;
  this->multipass_ = config.multipass;
  this->x_backlash_ = config.backlash;
  this->uv_ = config.uv;
  this->halftone_ = config.halftone;
  this->printing_strength_ = config.printing_strength;
  this->focus_ = config.focus;
  this->focus_step_ = config.focus_step;
  this->min_power_ = config.min_power;
  this->ink_ = config.ink;
  this->printing_speed_ = config.printing_speed;
  this->frequency_ = config.frequency;
  this->pulse_width_ = config.pulse_width;
  this->fill_interval_ = config.fill_interval;
  this->fill_angle_ = config.fill_angle;
  this->fill_bidirectional_ = config.fill_bidirectional;
  this->fill_hatch_ = config.fill_hatch;
}

// Clone

LayerPtr Layer::clone() {
  LayerPtr new_layer = std::make_shared<Layer>(document_, color(), name());
  new_layer->is_visible_ = this->is_visible_;
  new_layer->speed_ = this->speed_;
  new_layer->power_ = this->power_;
  new_layer->color_ = this->color_;
  new_layer->module_ = this->module_;
  new_layer->repeat_ = this->repeat_;
  new_layer->target_height_ = this->target_height_;
  new_layer->step_height_ = this->step_height_;
  new_layer->use_diode_ = this->use_diode_;
  new_layer->multipass_ = this->multipass_;
  new_layer->x_backlash_ = this->x_backlash_;
  new_layer->uv_ = this->uv_;
  new_layer->halftone_ = this->halftone_;
  new_layer->printing_strength_ = this->printing_strength_;
  new_layer->focus_ = this->focus_;
  new_layer->focus_step_ = this->focus_step_;
  new_layer->min_power_ = this->min_power_;
  new_layer->ink_ = this->ink_;
  new_layer->printing_speed_ = this->printing_speed_;
  new_layer->frequency_ = this->frequency_;
  new_layer->pulse_width_ = this->pulse_width_;
  new_layer->fill_interval_ = this->fill_interval_;
  new_layer->fill_angle_ = this->fill_angle_;
  new_layer->fill_bidirectional_ = this->fill_bidirectional_;
  new_layer->fill_hatch_ = this->fill_hatch_;
  children_mutex_.lock();
  for (auto &shape : children_) {
    new_layer->addShape(shape->clone());
  }
  children_mutex_.unlock();
  return new_layer;
}
