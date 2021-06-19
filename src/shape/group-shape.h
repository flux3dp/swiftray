#ifndef GROUPSHAPE_H
#define GROUPSHAPE_H

#include <shape/shape.h>
#include <canvas/cache-stack.h>

// We may need to change the group to cross-layer group instead of a shape object in the future
// This group implementation is downward compatible with other vector design software's paradigm
class GroupShape : public Shape {
public:
  GroupShape();

  GroupShape(QList<ShapePtr> &children);

  void calcBoundingBox() const override;

  bool hitTest(QPointF global_coord, qreal tolerance) const override;

  bool hitTest(QRectF global_coord_rect) const override;

  void cache() const;

  bool isParentSelected() const;

  void paint(QPainter *painter) const override;

  ShapePtr clone() const override;

  Shape::Type type() const override;

  const QList<ShapePtr> &children() const;

  CacheStack &cacheStack() const;

private:
  QList<ShapePtr> children_;
  mutable unique_ptr<CacheStack> cache_;
};

#endif // GROUPSHAPE_H
