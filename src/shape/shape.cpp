#include <QDebug>
#include <shape/shape.h>

#define SELECTION_TOLERANCE 15

using namespace std;

Shape::Shape() noexcept {
    rotation_ = 0;
    transform_ = QTransform();
    temp_transform_ = QTransform();
    selected = false;
    parent_ = nullptr;
}

Shape::~Shape() {}

// only calls this when the path is different

qreal Shape::x() const { return transform_.dx(); }

qreal Shape::y() const { return transform_.dy(); }

qreal Shape::rotation() const { return rotation_; }

QPointF Shape::pos() const { return QPointF(x(), y()); }

Layer *Shape::parent() const { return parent_; }

void Shape::setParent(Layer *parent) { parent_ = parent; }

void Shape::applyTransform(const QTransform &transform) {
    transform_ = transform_ * transform;
    bbox_need_recalc_ = true;
}

void Shape::setTransform(const QTransform &transform) {
    transform_ = transform;
    bbox_need_recalc_ = true;
}

void Shape::setRotation(qreal rotation) { rotation_ = rotation; }

void Shape::calcBoundingBox() const {
    qWarning() << "Shape::calcBoundingBox not implemented" << this;
}

const QTransform &Shape::transform() const { return transform_; }

bool Shape::hitTest(QPointF, qreal) const {
    qWarning() << "Shape::hitTest(point) not implemented" << this;
    return false;
}

bool Shape::hitTest(QRectF) const {
    qWarning() << "Shape::hitTest(rect) not implemented" << this;
    return false;
}

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

shared_ptr<Shape> Shape::clone() const {
    shared_ptr<Shape> shape = make_shared<Shape>(*this);
    qInfo() << "Clone Shape" << shape.get();
    return shape;
}

Shape::Type Shape::type() const { return Shape::Type::None; }

void Shape::invalidBBox() { bbox_need_recalc_ = true; }

void Shape::setTempTransform(const QTransform &transform) {
    temp_transform_ = transform;
}