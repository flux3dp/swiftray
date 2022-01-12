#pragma once

#include <QPainter>
#include <QPixmap>
#include <shape/shape.h>

class BitmapShape : public Shape {
public:
  BitmapShape();

  BitmapShape(QImage &image);

  BitmapShape(const BitmapShape &orig);

  ShapePtr clone() const override;

  bool hitTest(QPointF global_coord, qreal tolerance) const override;

  bool hitTest(QRectF global_coord_rect) const override;

  void paint(QPainter *painter) const override;

  Shape::Type type() const override;

  const QPixmap *pixmap() const;

  QImage &image() const;

  void invertPixels();

  void setGradient(bool gradient) { gradient_ = gradient; }
  bool gradient() { return gradient_; }
  void setThrshBrightness(int thrsh) { thrsh_brightness_ = thrsh; }
  int thrsh_brightness() { return thrsh_brightness_; }

  friend class DocumentSerializer;

private:
  void calcBoundingBox() const override;

  unique_ptr<QPixmap> bitmap_;
  mutable QImage tinted_image_;            // Cache object
  mutable std::uintptr_t tinted_signature; // Cache object

  bool gradient_; // gradient or binarized
  int thrsh_brightness_; // threshold value for binarization
};
