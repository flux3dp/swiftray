#include <QDebug>
#include <gcode/toolpath_exporter.h>
#include <iostream>

ToolpathExporter::ToolpathExporter(BaseGenerator *generator) noexcept {
    global_transform_ = QTransform();
    generator_ = generator;
}

void ToolpathExporter::convertStack(const QList<LayerPtr> &layers) {
    generator_->setLaserPowerLimit(0);
    generator_->turnOffLaser();
    generator_->home();
    
    for (auto &layer : layers) {
        convertLayer(layer);
    }
}

void ToolpathExporter::convertLayer(const LayerPtr &layer) {
    // Reset context states for the layer
    global_transform_ = QTransform();
    layer_polygons_.clear();
    layer_bitmap_ = QPixmap(QSize(3000, 2000));
    layer_painter_ = make_unique<QPainter>(&layer_bitmap_);
    qInfo() << "Output Layer #" << layer->name();

    //generator_->setLaserPowerLimit(layer->strength);

    for (auto &shape : layer->children()) {
        convertShape(shape);
    }

    //generator_->setLaserPowerLimit(layer->);

    layer_painter_->end();

    sortPolygons();

    outputLayerGcode();
    outputLayerPathGcode();
    outputLayerBitmapGcode();
}

void ToolpathExporter::convertShape(const ShapePtr &shape) {
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

void ToolpathExporter::convertGroup(const GroupShape *group) {
    for (auto &shape : group->children()) {
        convertShape(shape);
    }
}

void ToolpathExporter::convertBitmap(const BitmapShape* bmp) {
    layer_painter_->setTransform(global_transform_, false);
    layer_painter_->drawPixmap(0, 0, *bmp->pixmap());
}

void ToolpathExporter::convertPath(const PathShape* path) {
    layer_polygons_.append((global_transform_ * path->transform()).map(path->path()).toSubpathPolygons());
}
    
void ToolpathExporter::sortPolygons() {
    // TODO (Path order optimization)

}

void ToolpathExporter::outputLayerGcode() {
    // qInfo() << "set laser power" << layer_power;

}

void ToolpathExporter::outputLayerPathGcode() {
    // qInfo() << "layer path code"
    QPointF current_pos = QPointF();
    std::cout << "G1F200Z0" << std::endl;
    std::cout << "G90" << std::endl;
    for (auto &poly : layer_polygons_) {
        if (poly.size() == 0) continue;
        current_pos = poly.first();
        std::cout << "G1F2000X" << current_pos.x()/10.0F << "Y" << current_pos.y()/10.0F << std::endl;
        //std::cout << "TURN ON LASER" << std::endl;
        for (QPointF &point : poly) {
            if (current_pos == point) continue;
            std::cout << "G1";
            if (point.x() != current_pos.x()) std::cout << "X" << point.x()/10.0F;
            if (point.y() != current_pos.y()) std::cout << "Y" << point.y()/10.0F;
            current_pos = point;
            std::cout << std::endl;
        }
        //std::cout << "TURN OFF LASER" << std::endl;
    }
}

void ToolpathExporter::outputLayerBitmapGcode() {

}