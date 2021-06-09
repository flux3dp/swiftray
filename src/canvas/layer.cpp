#include <QDebug>
#include <canvas/layer.h>
#include <shape/shape.h>

const QString LayerColors[17] = {"#333333", "#3F51B5", "#F44336", "#FFC107", "#8BC34A",
                         "#2196F3", "#009688", "#FF9800", "#CDDC39", "#00BCD4",
                         "#FFEB3B", "#E91E63", "#673AB7", "#03A9F4", "#9C27B0",
                         "#607D8B", "#9E9E9E"};

int layer_color_counter;

Layer::Layer() {
    color_ = QColor(LayerColors[(layer_color_counter++) % 17]);
    name = "Layer 1";
}

Layer::Layer(int new_layer_id) {
    color_ = QColor(LayerColors[(new_layer_id-1) % 17]);
    name = "Layer " + QString::number(new_layer_id);
}

void Layer::paint(QPainter *painter, int counter) const {
    QPen dash_pen = QPen(color_, 2, Qt::DashLine);
    dash_pen.setDashPattern(QVector<qreal>(10, 3));
    dash_pen.setCosmetic(true);
    dash_pen.setDashOffset(counter);
    QPen solid_pen = QPen(color_, 2, Qt::SolidLine);
    solid_pen.setCosmetic(true);

    bool selected_flag = false;
    painter->setPen(solid_pen);
    // Draw shapes
    for (auto &shape : children_) {
        if (shape->selected() && !selected_flag) {
            painter->setPen(dash_pen);
            selected_flag = true;
        } else if (!shape->selected() && selected_flag) {
            painter->setPen(solid_pen);
            selected_flag = false;
        }
        shape->paint(painter);
    }
}

void Layer::addShape(ShapePtr &shape) {
    shape->setParent(this);
    children_.push_back(shape);
}

void Layer::removeShape(ShapePtr &shape) {
    shape->setParent(nullptr);
    children_.removeOne(shape);
}

void Layer::clear() { children_.clear(); }

QColor Layer::color() const { return color_; }

void Layer::setColor(QColor color) { color_ = color; }

QList<ShapePtr> &Layer::children() { return children_; }

LayerPtr Layer::clone() {
    LayerPtr layer = make_shared<Layer>();
    for (auto &shape : children_) {
        ShapePtr new_shape = shape->clone();
        layer->addShape(new_shape);
    }

    layer->name = name;
    layer->setColor(color());
    return layer;
}
