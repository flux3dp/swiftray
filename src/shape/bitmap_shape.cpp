#include <canvas/layer.h>
#include <shape/bitmap_shape.h>

BitmapShape::BitmapShape(QImage &image) {
    QImage grayscale = image.convertToFormat(QImage::Format_Grayscale8);
    bitmap_ = QPixmap::fromImage(grayscale);
}

bool BitmapShape::hitTest(QPointF global_coord, qreal tolerance) const {
    QPointF local_coord = transform().inverted().map(global_coord);
    return hitTest(QRectF(global_coord.x() - tolerance, global_coord.y() - tolerance, tolerance * 2, tolerance * 2));
}

bool BitmapShape::hitTest(QRectF global_coord_rect) const {
    return boundingRect().intersects(global_coord_rect);
}

QRectF BitmapShape::boundingRect() const {
    return transform().mapRect(bitmap_.rect());
}

void BitmapShape::paint(QPainter *painter) const {
    //TODO cache this
    QPainter p;

    QImage img = bitmap_.toImage();
    QImage mask(img);

    p.begin(&mask);
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.fillRect(QRect(0,0,mask.width(),mask.height()), parent()->color());
    p.end();

    p.begin(&img);
    p.setCompositionMode(QPainter::CompositionMode_ColorBurn);
    p.drawImage(0, 0, mask);
    p.end();

    painter->save();
    painter->setTransform(transform(), true);
    painter->drawImage(0, 0, img);
    painter->restore();
}

ShapePtr BitmapShape::clone() const {
    return make_shared<BitmapShape>(*this);
}

Shape::Type BitmapShape::type() const {
    return Shape::Type::Bitmap;
}