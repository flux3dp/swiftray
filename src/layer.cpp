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

Layer::Type Layer::type() const { return type_; }

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

double Layer::minPower() const { return min_power_; }

bool Layer::isOneWayEngraving() const { return is_one_way_engraving_; }

int Layer::module() const { return module_; }

int Layer::ink() const { return ink_; }

double Layer::printingSpeed() const { return printing_speed_; }

int Layer::multipass() const { return multipass_; }

int Layer::halftone() const { return halftone_; }

double Layer::amDensity() const { return am_density_; }

float Layer::printingStrength() const { return printing_strength_; }

double Layer::cRatio() const { return c_ratio_; }

double Layer::mRatio() const { return m_ratio_; }

double Layer::yRatio() const { return y_ratio_; }

double Layer::kRatio() const { return k_ratio_; }

float Layer::smooth() const { return smooth_; }

const QString& Layer::rawAmAngleMap() const { return raw_am_angle_map_; }

const QString& Layer::rawColorCurvesMap() const { return raw_color_curves_map_; }

int Layer::refreshInterval() const { return refresh_interval_; }

int Layer::refreshThreshold() const { return refresh_threshold_; }

int Layer::nozzleMode() const { return nozzle_mode_; }

double Layer::nozzleOffsetX() const { return nozzle_offset_x_; }

double Layer::nozzleOffsetY() const { return nozzle_offset_y_; }

float Layer::focus() const { return focus_; }

float Layer::focusStep() const { return focus_step_; }

double Layer::ceZLimit() const { return ce_z_limit_; }

int Layer::interpolation() const { return interpolation_; }

double Layer::rightPadding() const { return right_padding_; }

int Layer::uvPrintingRepeat() const { return uv_printing_repeat_; }

int Layer::uvCuringAfter() const { return uv_curing_after_; }

int Layer::uvCuringRepeat() const { return uv_curing_repeat_; }

int Layer::uvStrength() const { return uv_strength_; }

int Layer::uvXStep() const { return uv_x_step_; }

int Layer::frequency() const { return frequency_; }

int Layer::pulseWidth() const { return pulse_width_; }

double Layer::fillInterval() const { return fill_interval_; }

double Layer::fillAngle() const { return fill_angle_; }

bool Layer::fillBidirectional() const { return fill_bidirectional_; }

bool Layer::fillHatch() const { return fill_hatch_; }

int Layer::dottingTime() const { return dotting_time_; }

double Layer::wobbleStep() const { return wobble_step_; }

double Layer::wobbleDiameter() const { return wobble_diameter_; }

int Layer::airAssist() const { return air_assist_; }

const QString& Layer::rawBBox() const { return raw_bbox_; }

int Layer::laserDelay() const { return laser_delay_; }

int Layer::dpmm() const { return dpmm_; }

bool Layer::isHighQuality() const { return is_high_quality_; }

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
  this->repeat_ = config.repeat;
  this->target_height_ = config.height;
  this->step_height_ = config.z_step;
  this->use_diode_ = config.diode;
  this->x_backlash_ = config.backlash;

  this->min_power_ = config.min_power;
  this->is_one_way_engraving_ = config.is_one_way_engraving;
  this->module_ = config.module;
  this->ink_ = config.ink;
  this->printing_speed_ = config.printing_speed;
  this->multipass_ = config.multipass;
  this->halftone_ = config.halftone;
  this->am_density_ = config.am_density;
  this->printing_strength_ = config.printing_strength;
  this->c_ratio_ = config.c_ratio;
  this->m_ratio_ = config.m_ratio;
  this->y_ratio_ = config.y_ratio;
  this->k_ratio_ = config.k_ratio;
  this->smooth_ = config.smooth;
  this->raw_am_angle_map_ = config.raw_am_angle_map;
  this->raw_color_curves_map_ = config.raw_color_curves_map;
  this->refresh_interval_ = config.refresh_interval;
  this->refresh_threshold_ = config.refresh_threshold;
  this->nozzle_mode_ = config.nozzle_mode;
  this->nozzle_offset_x_ = config.nozzle_offset_x;
  this->nozzle_offset_y_ = config.nozzle_offset_y;
  this->focus_ = config.focus;
  this->focus_step_ = config.focus_step;
  this->ce_z_limit_ = config.ce_z_limit;
  this->interpolation_ = config.interpolation;
  this->right_padding_ = config.right_padding;
  this->uv_printing_repeat_ = config.uv_printing_repeat;
  this->uv_curing_after_ = config.uv_curing_after;
  this->uv_curing_repeat_ = config.uv_curing_repeat;
  this->uv_strength_ = config.uv_strength;
  this->uv_x_step_ = config.uv_x_step;
  this->frequency_ = config.frequency;
  this->pulse_width_ = config.pulse_width;
  this->fill_interval_ = config.fill_interval;
  this->fill_angle_ = config.fill_angle;
  this->fill_bidirectional_ = config.fill_bidirectional;
  this->fill_hatch_ = config.fill_hatch;
  this->dotting_time_ = config.dotting_time;
  this->wobble_step_ = config.wobble_step;
  this->wobble_diameter_ = config.wobble_diameter;
  this->air_assist_ = config.air_assist;
  this->raw_bbox_ = config.raw_bbox;
  this->laser_delay_ = config.laser_delay;
  this->dpmm_ = config.dpmm;
  this->is_high_quality_ = config.is_high_quality;
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
  children_mutex_.lock();
  for (auto &shape : children_) {
    new_layer->addShape(shape->clone());
  }
  children_mutex_.unlock();
  return new_layer;
}
