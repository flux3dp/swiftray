#pragma once

#include <shape/shape.h>
#include <canvas/cache-stack.h>

// We may need to change the group to cross-layer group instead of a shape object in the future
// This group implementation is downward compatible with other vector design software's paradigm
class GroupShape : public Shape {
public:
  GroupShape();

  GroupShape(QList<ShapePtr> &children);

  bool hitTest(QPointF global_coord, qreal tolerance) const override;

  bool hitTest(QRectF global_coord_rect) const override;

  void paint(QPainter *painter) const override;

  ShapePtr clone() const override;

  Shape::Type type() const override;

  const QList<ShapePtr> &children() const;

  friend class DocumentSerializer;

  friend class CacheStack;

protected:

  CacheStack &cacheStack() const;

private:

  void calcBoundingBox() const override;

  void cache() const;


  QList<ShapePtr> children_;
  mutable unique_ptr<CacheStack> cache_;
};
