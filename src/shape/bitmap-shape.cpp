#include <QDebug>
#include <opencv2/imgcodecs/imgcodecs.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <layer.h>
#include <shape/bitmap-shape.h>


BitmapShape::BitmapShape() : Shape()
{
}

/**
 * @param image Must be Format_ARGB32 grayscaled image
 */
BitmapShape::BitmapShape(const QImage &image) : Shape(),
  src_image_(image)
{
}

/**
 * @param image Must be Format_ARGB32 grayscaled image
 */
BitmapShape::BitmapShape(QImage &&image) : Shape(),
  src_image_(std::move(image))
{
}

BitmapShape::BitmapShape(const BitmapShape &orig) : Shape(orig),
  src_image_(orig.src_image_)
{
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
  image_rect.addRect(src_image_.rect());
  return image_rect.intersects(local_rect);
}

void BitmapShape::calcBoundingBox() const {
  bbox_ = transform().mapRect(src_image_.rect());
  rotated_bbox_ = transform().map(QPolygonF(QRectF(src_image_.rect())));
}

/**
 * @brief Return the image to be painted on canvas (apply the layer color and gradient settings)
 *        Adjusted from the source image
 * @return
 */
const QImage &BitmapShape::imageForDisplay() const {
  std::uintptr_t parent_color = hasLayer() ? layer()->color().value() : 0;
  std::uintptr_t gradient_switch = gradient_;
  std::uintptr_t thrsh = thrsh_brightness_;
  if (dirty_ || (tinted_signature != parent_color + gradient_switch + thrsh) ) {
    QColor layer_color = layer()->color();
    tinted_signature = parent_color + gradient_switch + thrsh;
    dirty_ = false;
    qInfo() << "Tinted image" << tinted_signature;

    // Update tinted image
    tinted_image_ = QImage(src_image_.size(), QImage::Format_ARGB32);

    if ( ! gradient_) {
      for (int y = 0; y < src_image_.height(); ++y) {
        for (int x = 0; x < src_image_.width(); ++x) {
          QRgb gray = qRed(src_image_.pixel(x, y)); // R = G = B = Gray
          // fill with layer color but also consider the alpha from pixel
          QRgb fill_color = qRgba(layer()->color().red(),
                                  layer()->color().green(),
                                  layer()->color().blue(),
                                  qAlpha(src_image_.pixel(x, y)));
          QRgb bm_p = gray < thrsh_brightness_ ? fill_color : qRgba(255, 255, 255, 0);
          tinted_image_.setPixel(x, y, bm_p);
        }
      }
    } else {
      for (int y = 0; y < src_image_.height(); ++y) {
        for (int x = 0; x < src_image_.width(); ++x) {
          int gray = qRed(src_image_.pixel(x, y)); // R = G = B = Gray
          // Force transparent for pure white pixel
          int alpha = gray == 255 ? 0 : qAlpha(src_image_.pixel(x, y));
          // Blend layer color and grayscale value
          // TBD: Select a proper blending algorithm
          QRgb bm_p = qRgba((layer_color.red() + (255 - layer_color.red()) * gray / 255),
                      layer_color.green() + (255 - layer_color.green()) * gray / 255,
                      layer_color.blue() + (255 - layer_color.blue()) * gray / 255,
                      alpha);
          tinted_image_.setPixel(x, y, bm_p);
        }
      }
    }

  }
  return tinted_image_;
}

void BitmapShape::invertPixels() {
  src_image_.invertPixels(QImage::InvertRgb);
  dirty_ = true;
}

void BitmapShape::paint(QPainter *painter) const {
  painter->save();
  painter->setTransform(temp_transform_, true);
  painter->setTransform(transform(), true);
  painter->drawImage(0, 0, imageForDisplay());
  // TODO (pass selection state with set pen or no pen?)
  painter->drawRect(src_image_.rect());
  painter->restore();
}

ShapePtr BitmapShape::clone() const {
  ShapePtr new_shape = std::make_shared<BitmapShape>(*this);
  return new_shape;
}

Shape::Type BitmapShape::type() const { return Shape::Type::Bitmap; }

const QImage &BitmapShape::sourceImage() const {
  return src_image_;
}
