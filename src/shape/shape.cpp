#include "shape.hpp"
#include <QDebug>

#define SELECTION_TOLERANCE 15

using namespace std;

Shape::Shape() noexcept {
    transform_ = QTransform();
    selected = false;
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

bool Shape::testHit(QPointF, qreal) const {
    qWarning() << "Shape::TestHit Point not implemented" << this;
    return false;
}

bool Shape::testHit(QRectF) const {
    qWarning() << "Shape::TestHit Rect not implemented" << this;
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
    shared_ptr<Shape> shape(new Shape(*this));
    qInfo() << "Clone Shape" << shape.get();
    return shape;
}
