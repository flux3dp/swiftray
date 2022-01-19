#pragma once

#include <QPainter>
#include <QPainterPath>
#include <QRectF>

class Layer;

class DocumentSerializer;

/**
    \class Shape
    \brief A base class for shape objects that contains transform and parent information
*/
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

  /**
   * @return Raw transform
   */
  const QTransform &transform() const;

  const QTransform &tempTransform() const;

  /**
   * @return The transform combines with group parents' transform (if any)
   */
  QTransform globalTransform() const;

  /**
   * Combine current transform with a new transform.
   * @param transform The new transform
   */
  void applyTransform(const QTransform &transform);

  /**
   * Set current transform to a new transform.
   * @param transform The new transform
   */
  void setTransform(const QTransform &transform);

  /**
   * Set temporarily transform that hasn't been commited during moving / rotating / scaling stage for displaying objects.
   * @param transform The new transform
   */
  void setTempTransform(const QTransform &transform);

  bool hasLayer() const;

  bool isParentSelected() const;

  /** Whether the layer of the object is locked */
  bool isLayerLocked() const;

  virtual std::shared_ptr<Shape> clone() const;

  virtual bool hitTest(QPointF global_coord, qreal tolerance) const;

  virtual bool hitTest(QRectF global_coord_rect) const;

  virtual void paint(QPainter *painter) const;

  /** Returns shape type in Shape::Type */
  virtual Type type() const;

  virtual operator QString();

  friend class DocumentSerializer;

protected:
  /** Calculate bounding box of the shape if the cache is invalid */
  virtual void calcBoundingBox() const;

private:
  /** Usually if shape belongs to a layer then the parent is null */
  Layer *layer_;
  /** Usually if shape belongs to a parent (group) then the layer is null */
  Shape *parent_;
  /** Rotation stores how the objects has been rotated, so the user can reverse the rotation after some operations */
  qreal rotation_;
  /** A flag indicate whether the bounding box is currently valid */
  mutable bool bbox_need_recalc_;

protected:
  /** Temporarily transform that hasn't been commited during moving / rotating / scaling stage for displaying objects. */
  QTransform temp_transform_;
  /** Shape transform including offset, scale, rotate and kew */
  QTransform transform_;
  /** Cached bounding box with rotation */
  mutable QPolygonF rotated_bbox_;
  /** Cached bounding box */
  mutable QRectF bbox_;
  /** Whether the object is selected by user */
  bool selected_;
};

typedef std::shared_ptr<Shape> ShapePtr;