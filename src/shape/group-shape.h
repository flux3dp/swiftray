#ifndef GROUPSHAPE_H
#define GROUPSHAPE_H

#include <shape/shape.h>
#include <canvas/cache-stack.h>

class GroupShape : public Shape {
public:
  GroupShape();

  GroupShape(QList<ShapePtr> &children);

  void calcBoundingBox() const override;

  bool hitTest(QPointF global_coord, qreal tolerance) const override;

  bool hitTest(QRectF global_coord_rect) const override;

  void cache() const;

  void paint(QPainter *painter) const override;

  ShapePtr clone() const override;

  Shape::Type type() const override;

  const QList<ShapePtr> &children() const;

private:
  QList<ShapePtr> children_;
  mutable CacheStack cache_stack_;
};

#endif // GROUPSHAPE_H
