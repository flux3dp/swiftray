#include <QDebug>
#include <layer.h>
#include <shape/bitmap-shape.h>

BitmapShape::BitmapShape() : Shape(), tinted_signature(0) {

}

BitmapShape::BitmapShape(QImage &image) : Shape(), tinted_signature(0) {
  // Process transparent image grayscale
  for (int yy = 0; yy < image.height(); yy++) {
    uchar *scan = image.scanLine(yy);
    int depth = 4;
    for (int xx = 0; xx < image.width(); xx++) {
      QRgb *rgbpixel = reinterpret_cast<QRgb *>(scan + xx * depth);
      int gray = qGray(*rgbpixel);
      *rgbpixel = QColor(gray, gray, gray, qAlpha(*rgbpixel)).rgba();
    }
  }
  bitmap_ = make_unique<QPixmap>(QPixmap::fromImage(image));
}

BitmapShape::BitmapShape(const BitmapShape &orig) : Shape(orig), tinted_signature(0) {
  bitmap_ = make_unique<QPixmap>(*orig.bitmap_);
  setLayer(orig.layer());
  setTransform(orig.transform());
}

bool BitmapShape::hitTest(QPointF global_coord, qreal tolerance) const {
  return hitTest(QRectF(global_coord.x() - tolerance,
                        global_coord.y() - tolerance, tolerance * 2,
                        tolerance * 2));
}

bool BitmapShape::hitTest(QRectF global_coord_rect) const {
  QPainterPath local_rect;
  local_rect.addRect(global_coord_rect);
  local_rect = transform().inverted().map(local_rect);
  QPainterPath image_rect;
  image_rect.addRect(image().rect());
  return image_rect.intersects(local_rect);
}

void BitmapShape::calcBoundingBox() const {
  bbox_ = transform().mapRect(bitmap_->rect());
  rotated_bbox_ = transform().map(QPolygonF(QRectF(bitmap_->rect())));
}

QImage &BitmapShape::image() const {
  assert(bitmap_.get() != nullptr);
  std::uintptr_t parent_color = hasLayer() ? 0 : layer()->color().value();
  std::uintptr_t bitmap_address =
       reinterpret_cast<std::uintptr_t>(bitmap_.get());
  if (tinted_signature != parent_color + bitmap_address) {
    tinted_signature = parent_color + bitmap_address;
    qInfo() << "Tinted image" << tinted_signature;
    tinted_image_ = bitmap_->toImage();
    QImage mask(tinted_image_);
    QPainter p;
    p.begin(&mask);
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.fillRect(QRect(0, 0, mask.width(), mask.height()), layer()->color());
    p.end();

    p.begin(&tinted_image_);
    p.setCompositionMode(QPainter::CompositionMode_SoftLight);
    p.drawImage(0, 0, mask);
    p.end();
  }
  return tinted_image_;
}

void BitmapShape::paint(QPainter *painter) const {
  painter->save();
  painter->setTransform(temp_transform_, true);
  painter->setTransform(transform(), true);
  painter->drawImage(0, 0, image());
  // TODO (pass selection state with set pen or no pen?)
  painter->drawRect(image().rect());
  painter->restore();
}

ShapePtr BitmapShape::clone() const {
  ShapePtr new_shape = make_shared<BitmapShape>(*this);
  return new_shape;
}

Shape::Type BitmapShape::type() const { return Shape::Type::Bitmap; }

const QPixmap *BitmapShape::pixmap() const { return bitmap_.get(); }