#include <shape/shape.hpp>
#include "layer.hpp"

Layer::Layer() {
    color_ = QColor::fromRgb(rand() % 256, rand() % 256, rand() % 256, 255);
    name = "Layer 1";
}

void Layer::paint(QPainter *painter, int counter) const {
    QPen dash_pen = QPen(color_, 2, Qt::DashLine);
    dash_pen.setDashPattern(QVector<qreal>(10, 3));
    dash_pen.setCosmetic(true);
    dash_pen.setDashOffset(counter);
    QPen solid_pen = QPen(color_, 2, Qt::SolidLine);
    solid_pen.setCosmetic(true);

    // Draw shapes
    for (int i = 0; i < children_.size(); i++) {
        if (children_.at(i)->selected) {
            painter->setPen(dash_pen);
        } else {
            painter->setPen(solid_pen);
        }

        children_.at(i)->paint(painter);
    }
}


void Layer::addShape(ShapePtr shape) {
    children_.push_back(shape);
}

void Layer::removeShape(ShapePtr shape) {
    children_.removeOne(shape);
}

void Layer::clear() {
    children_.clear();
}

QColor Layer::color() {
    return color_;
}

void Layer::setColor(QColor color) {
    color_ = color;
}

QList<ShapePtr> &Layer::children() {
    return children_;
}

Layer Layer::clone() {
    Layer layer;

    for (ShapePtr &shape : children_) {
        layer.children() << shape->clone();
    }

    layer.name = name;
    layer.setColor(color());
    return layer;
}
