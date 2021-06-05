#include <shape/shape.h>
#include <QDebug>

#define SELECTION_TOLERANCE 15

using namespace std;

Shape::Shape() noexcept {
    transform_ = QTransform();
    selected = false;
    parent_ = nullptr;
}

Shape::~Shape() {
}

// only calls this when the path is different

qreal Shape::x() const {
    return transform_.dx();
}

qreal Shape::y() const {
    return transform_.dy();
}

qreal Shape::scaleX() const {
    return transform_.m22();
}

qreal Shape::scaleY() const {
    return transform_.m23();
}

QPointF Shape::pos() const {
    return QPointF(x(), y());
}

Layer* Shape::parent() const {
    return parent_;
}

void Shape::setParent(Layer* parent) {
    parent_ = parent;
}

void Shape::applyTransform(QTransform transform) {
    transform_ = transform_ * transform;
}

void Shape::setTransform(QTransform transform) {
    transform_ = transform;
}

QTransform Shape::transform() const {
    return transform_;
}


void Shape::cacheSelectionTestingData() {
    qWarning() << "Shape::CacheSelection not implemented" << this;
}


void Shape::simplify() {
    qWarning() << "Shape::Simplify not implemented" << this;
    return;
}

bool Shape::hitTest(QPointF, qreal) const {
    qWarning() << "Shape::hitTest Point not implemented" << this;
    return false;
}

bool Shape::hitTest(QRectF) const {
    qWarning() << "Shape::hitTest Rect not implemented" << this;
    return false;
}

QRectF Shape::boundingRect() const {
    qWarning() << "Shape::Bounding rect not implemented" << this;
    return QRectF();
}

void Shape::paint(QPainter *) const {
    qWarning() << "Shape::Paint not implemented" << this;
}

shared_ptr<Shape> Shape::clone() const {
    shared_ptr<Shape> shape = make_shared<Shape>(*this);
    qInfo() << "Clone Shape" << shape.get();
    return shape;
}

Shape::Type Shape::type() const {
    return Shape::Type::None;
}
