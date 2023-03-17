#include <QDebug>
#include <document.h>
#include <layer.h>
#include <shape/shape.h>
#include <shape/group-shape.h>
#include <shape/bitmap-shape.h>
#include <shape/text-shape.h>
#include <shape/path-shape.h>


#define SELECTION_TOLERANCE 15

Shape::Shape() noexcept:
     rotation_(0),
     transform_(),
     temp_transform_(),
     selected_(false),
     layer_(nullptr),
     parent_(nullptr),
     bbox_need_recalc_(false),
     filled_(false) {}

Shape::~Shape() {
  //qDebug() << "[Memory] ~Shape" << this;
}

qreal Shape::x() const { return transform_.dx(); }

qreal Shape::y() const { return transform_.dy(); }

qreal Shape::rotation() const { return rotation_; }

QPointF Shape::pos() const { return QPointF(x(), y()); }

Layer *Shape::layer() const {
  Shape *top_node = (Shape *) this;
  while (top_node->parent_ != nullptr) {
    top_node = top_node->parent_;
  }
  Layer *top_layer = top_node->layer_;
  Q_ASSERT_X(top_layer != nullptr, "Shape",
             "This node or its top node doesn't belong to any layer, the logic may be wrong."
             "You can also use shape->hasLayer() to check before access.");
  return top_layer;
}

bool Shape::selected() const { return selected_; }

void Shape::setLayer(Layer *layer) {
  layer_ = layer;
}

// Note: hasLayer() does not consider if parent node has layers.
bool Shape::hasLayer() const { return layer_ != nullptr; }

void Shape::applyTransform(const QTransform &transform) {
  transform_ = transform_ * transform;
  flushCache();
}

void Shape::setTransform(const QTransform &transform) {
  transform_ = transform;
  flushCache();
}

void Shape::setRotation(qreal rotation) { rotation_ = rotation; }

void Shape::setSelected(bool selected) { selected_ = selected; }

void Shape::calcBoundingBox() const {
  qWarning() << "Shape::calcBoundingBox not implemented" << this;
}

const QTransform &Shape::transform() const { return transform_; }

const QTransform &Shape::tempTransform() const { return temp_transform_; }

QTransform Shape::globalTransform() const {
  QTransform global_transform = transform_;
  Shape *top_node = (Shape *) this;
  while (top_node->parent() != nullptr) {
    top_node = top_node->parent();
    global_transform = global_transform * top_node->transform();
  }
  return global_transform * top_node->temp_transform_;
}

bool Shape::hitTest(QPointF, qreal) const {
  qWarning() << "Shape::hitTest(point) not implemented" << this;
  return false;
}

bool Shape::hitTest(QRectF) const {
  qWarning() << "Shape::hitTest(rect) not implemented" << this;
  return false;
}

/**
 * @brief The bounding rect of the shape on canvas
 *        NOTE: translation and rotation have already been considered
 * 
 * @return QRectF 
 */
QRectF Shape::boundingRect() const {
  if (bbox_need_recalc_) {
    calcBoundingBox();
    bbox_need_recalc_ = false;
  }
  return bbox_;
}

QPolygonF Shape::rotatedBBox() const {
  if (bbox_need_recalc_) {
    calcBoundingBox();
    bbox_need_recalc_ = false;
  }
  return rotated_bbox_;
}

void Shape::paint(QPainter *) const {
  qWarning() << "Shape::Paint not implemented" << this;
}

std::shared_ptr<Shape> Shape::clone() const {
  std::shared_ptr<Shape> shape = std::make_shared<Shape>(*this);
  qInfo() << "Clone Shape" << shape.get();
  return shape;
}

Shape::Type Shape::type() const { return Shape::Type::None; }

void Shape::flushCache() {
  bbox_need_recalc_ = true;
  if (layer_) {
    layer_->flushCache();
  }
  /*if (parent_) {
    parent_->flushCache();
  }*/
}

void Shape::setTempTransform(const QTransform &transform) {
  temp_transform_ = transform;
  flushCache();
}

Shape *Shape::parent() const {
  return parent_;
}

void Shape::setParent(Shape *parent) {
  parent_ = parent;
}

Shape::operator QString() {
  return "Shape(" + QString::number((int) type()) + ")";
}

bool Shape::isParentSelected() const {
  bool result = selected();
  Shape *p = parent();
  while (p != nullptr) {
    result = p->selected();
    p = p->parent();
  }
  return result;
}

bool Shape::isLayerLocked() const {
  return this->hasLayer() && layer_->isLocked();
}

bool Shape::isFilled() const {
  return filled_;
}

void Shape::setFilled(bool filled) {
  filled_ = filled;
}