#pragma once

#include <QPainter>
#include <QPixmap>
#include <shape/shape.h>

class BitmapShape : public Shape {
public:
  BitmapShape();

  BitmapShape(const QImage &image);

  BitmapShape(QImage &&image);

  BitmapShape(const BitmapShape &orig);

  ShapePtr clone() const override;

  bool hitTest(QPointF global_coord, qreal tolerance) const override;

  bool hitTest(QRectF global_coord_rect) const override;

  void paint(QPainter *painter) const override;

  Shape::Type type() const override;

  const QImage &imageForDisplay() const;
  const QImage &sourceImage() const;

  void invertPixels();

  void setGradient(bool gradient) { gradient_ = gradient; }
  bool gradient() const { return gradient_; }
  void setThrshBrightness(int thrsh) { thrsh_brightness_ = thrsh; }
  int thrsh_brightness() const { return thrsh_brightness_; }

  friend class DocumentSerializer;

private:
  void calcBoundingBox() const override;
  int getLuminanceFromQRgb(QRgb rgb);

  mutable QImage src_image_;
  mutable bool dirty_ = true;                  // Force an update for Cache object
  mutable QImage tinted_image_;                // Cache object
  mutable std::uintptr_t tinted_signature = 0; // Cache object

  bool gradient_ = true; // gradient or binarized
  int thrsh_brightness_ = 128; // threshold value for binarization
};
