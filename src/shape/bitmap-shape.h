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

  friend class DocumentSerializer;

private:
  void calcBoundingBox() const override;

  std::unique_ptr<QPixmap> bitmap_;
  mutable QImage tinted_image_;            // Cache object
  mutable std::uintptr_t tinted_signature; // Cache object
};
