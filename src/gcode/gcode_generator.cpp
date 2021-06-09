#include <QDebug>
#include <gcode/gcode_generator.h>
#include <iostream>

GCodeGenerator::GCodeGenerator() noexcept {
    global_transform_ = QTransform();
}

void GCodeGenerator::convertStack(const QList<LayerPtr> &layers) {
    for (auto &layer : layers) {
        convertLayer(layer);
    }
}

void GCodeGenerator::convertLayer(const LayerPtr &layer) {
    // Reset context states for the layer
    global_transform_ = QTransform();
    layer_polygons_.clear();
    layer_bitmap_ = QPixmap(QSize(3000, 2000));
    layer_painter_ = make_unique<QPainter>(&layer_bitmap_);
    qInfo() << "Output Layer #" << layer->name;

    for (auto &shape : layer->children()) {
        convertShape(shape);
    }

    layer_painter_->end();

    sortPolygons();

    outputLayerGcode();
    outputLayerPathGcode();
    outputLayerBitmapGcode();
}

void GCodeGenerator::convertShape(const ShapePtr &shape) {
    switch (shape->type()) {
    case Shape::Type::Group:
        convertGroup(static_cast<GroupShape *>(shape.get()));
        break;
    case Shape::Type::Bitmap:
        convertBitmap(static_cast<BitmapShape *>(shape.get()));
        break;
    case Shape::Type::Path:
    case Shape::Type::Text:
        convertPath(static_cast<PathShape *>(shape.get()));
        break;
    }
}

void GCodeGenerator::convertGroup(const GroupShape *group) {
    for (auto &shape : group->children()) {
        convertShape(shape);
    }
}

void GCodeGenerator::convertBitmap(const BitmapShape* bmp) {
    layer_painter_->setTransform(global_transform_, false);
    layer_painter_->drawPixmap(0, 0, *bmp->pixmap());
}

void GCodeGenerator::convertPath(const PathShape* path) {
    layer_polygons_.append((global_transform_ * path->transform()).map(path->path()).toSubpathPolygons());
}
    
void GCodeGenerator::sortPolygons() {
    // TODO (Path order optimization)

}

void GCodeGenerator::outputLayerGcode() {
    // qInfo() << "set laser power" << layer_power;

}

void GCodeGenerator::outputLayerPathGcode() {
    // qInfo() << "layer path code"
    QPointF current_pos = QPointF();
    for (auto &poly : layer_polygons_) {
        if (poly.size() == 0) continue;
        current_pos = poly.first();
        std::cout << "G1X" << current_pos.x() << "Y" << current_pos.y() << std::endl;
        std::cout << "TURN ON LASER" << std::endl;
        for (QPointF &point : poly) {
            std::cout << "G1";
            if (point.x() != current_pos.x()) std::cout << "X" << point.x();
            if (point.y() != current_pos.y()) std::cout << "Y" << point.y();
            current_pos = point;
            std::cout << std::endl;
        }
        std::cout << "TURN OFF LASER" << std::endl;
    }
}

void GCodeGenerator::outputLayerBitmapGcode() {

}