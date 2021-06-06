#include <QDebug>
#include <canvas/layer.h>
#include <shape/bitmap_shape.h>

BitmapShape::BitmapShape(QImage &image) : Shape() {
    QImage grayscale = image.convertToFormat(QImage::Format_Grayscale8);
    bitmap_ = make_unique<QPixmap>(QPixmap::fromImage(grayscale));
}

BitmapShape::BitmapShape(const BitmapShape &orig) : Shape() {
    bitmap_ = make_unique<QPixmap>(*orig.bitmap_);
    setParent(orig.parent());
    setTransform(orig.transform());
}

bool BitmapShape::hitTest(QPointF global_coord, qreal tolerance) const {
    QPointF local_coord = transform().inverted().map(global_coord);
    return hitTest(QRectF(global_coord.x() - tolerance, global_coord.y() - tolerance, tolerance * 2, tolerance * 2));
}

bool BitmapShape::hitTest(QRectF global_coord_rect) const {
    return boundingRect().intersects(global_coord_rect);
}

void BitmapShape::calcBoundingBox() const {
    bbox_ = transform().mapRect(bitmap_->rect());
    rotated_bbox_ = transform().map(QPolygonF(QRectF(bitmap_->rect())));
}

QImage &BitmapShape::image() const {
    std::uintptr_t parent_color = parent() == nullptr ? 0 : parent()->color().value();
    std::uintptr_t bitmap_address = reinterpret_cast<std::uintptr_t>(bitmap_.get());
    if (tinted_signature != parent_color + bitmap_address) {
        tinted_signature = parent_color + bitmap_address;
        qInfo() << "Tinted image" << tinted_signature;
        tinted_image_ = bitmap_->toImage();
        QImage mask(tinted_image_);
        QPainter p;
        p.begin(&mask);
        p.setCompositionMode(QPainter::CompositionMode_SourceIn);
        p.fillRect(QRect(0,0,mask.width(),mask.height()), parent()->color());
        p.end();

        p.begin(&tinted_image_);
        p.setCompositionMode(QPainter::CompositionMode_ColorBurn);
        p.drawImage(0, 0, mask);
        p.end();
    }
    return tinted_image_;
}

void BitmapShape::paint(QPainter *painter) const{
    painter->save();
    painter->setTransform(temp_transform_, true);
    painter->setTransform(transform(), true);
    painter->drawImage(0, 0, image());
    painter->restore();
}

ShapePtr BitmapShape::clone() const {
    ShapePtr new_shape = make_shared<BitmapShape>(*this);
    return new_shape;
}

Shape::Type BitmapShape::type() const {
    return Shape::Type::Bitmap;
}