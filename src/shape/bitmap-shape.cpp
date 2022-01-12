#include <QDebug>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <layer.h>
#include <shape/bitmap-shape.h>


BitmapShape::BitmapShape() :
    Shape(), tinted_signature(0), gradient_(true), thrsh_brightness_(128) {

}

BitmapShape::BitmapShape(QImage &image) :
    Shape(), tinted_signature(0), gradient_(true), thrsh_brightness_(128) {
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

BitmapShape::BitmapShape(const BitmapShape &orig) :
    Shape(orig), tinted_signature(0), gradient_(true), thrsh_brightness_(128) {
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

/**
 * @brief Return the image to be painted on canvas (apply the layer color and gradient settings)
 *        Not the origin image
 * @return
 */
QImage &BitmapShape::image() const {
  assert(bitmap_.get() != nullptr);
  std::uintptr_t parent_color = hasLayer() ? layer()->color().value() : 0;
  std::uintptr_t bitmap_address =
       reinterpret_cast<std::uintptr_t>(bitmap_.get());
  std::uintptr_t gradient_switch = gradient_;
  std::uintptr_t thrsh = thrsh_brightness_;
  if (tinted_signature != parent_color + bitmap_address + gradient_switch + thrsh) {
    tinted_signature = parent_color + bitmap_address + gradient_switch + thrsh;
    qInfo() << "Tinted image" << tinted_signature;
    tinted_image_ = bitmap_->toImage();

    if ( ! gradient_) {
      bool apply_alpha = tinted_image_.hasAlphaChannel();
      for (int y = 0; y < tinted_image_.height(); ++y) {
        for (int x = 0; x < tinted_image_.width(); ++x) {
          QRgb pixel = tinted_image_.pixel(x, y);
          int grayscale_val = qGray(pixel);
          QRgb bm_p;
          if (apply_alpha) {
            grayscale_val = 255 - (255 - grayscale_val) * qAlpha(pixel) / 255;
          }
          bm_p = grayscale_val < thrsh_brightness_ ? layer()->color().rgb() : qRgb(255, 255, 255);
          tinted_image_.setPixel(x, y, bm_p);
        }
      }
    } else {
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

  }
  return tinted_image_;
}

void BitmapShape::invertPixels() {
  image().invertPixels(QImage::InvertRgb);
  bitmap_ = make_unique<QPixmap>(QPixmap::fromImage(tinted_image_));
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
