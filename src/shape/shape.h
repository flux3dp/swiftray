#include <QPainter>
#include <QPainterPath>
#include <QRectF>

#ifndef SHAPE_H
#define SHAPE_H

using namespace std;

class Layer;

class Shape {
public:
  enum class Type {
    None, Path, Bitmap, Text, Group
  };

  Shape() noexcept;

  virtual ~Shape();

  // General attributes
  bool selected() const;

  Layer *layer() const;

  QPointF pos() const;

  qreal rotation() const;

  qreal x() const;

  qreal y() const;

  Shape *parent() const;

  void setRotation(qreal r);

  void setLayer(Layer *layer);

  void setSelected(bool selected);

  void setParent(Shape *parent);


  // Bounding box
  QRectF boundingRect() const;

  QPolygonF rotatedBBox() const;

  void flushCache();

  // Transform related
  const QTransform &transform() const;

  const QTransform &tempTransform() const;

  QTransform globalTransform() const;

  void applyTransform(const QTransform &transform);

  void setTransform(const QTransform &transform);

  void setTempTransform(const QTransform &transform);

  bool hasLayer() const;

  // Virtual functions
  virtual void calcBoundingBox() const;

  virtual shared_ptr<Shape> clone() const;

  virtual bool hitTest(QPointF global_coord, qreal tolerance) const;

  virtual bool hitTest(QRectF global_coord_rect) const;

  virtual void paint(QPainter *painter) const;

  virtual Type type() const;

  virtual operator QString();

private:
  Layer *layer_;
  Shape *parent_;
  qreal rotation_;
  mutable bool bbox_need_recalc_;

protected:
  QTransform temp_transform_;
  QTransform transform_;
  mutable QPolygonF rotated_bbox_;
  mutable QRectF bbox_;
  bool selected_;
};

typedef shared_ptr<Shape> ShapePtr;
#endif // SHAPE_H
