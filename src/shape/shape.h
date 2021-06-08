#include <QPainter>
#include <QPainterPath>
#include <QRectF>

#ifndef SHAPE_H
#define SHAPE_H

using namespace std;
class Layer;
class Shape {
  public:
    enum class Type { None, Path, Bitmap, Text, Group };
    Shape() noexcept;
    virtual ~Shape();

    // General attributes
    bool selected;
    qreal x() const;
    qreal y() const;
    QPointF pos() const;
    qreal rotation() const;
    Layer *parent() const;
    void setX(qreal x);
    void setY(qreal y);
    void setPos(QPointF pos);
    void setRotation(qreal r);
    void setParent(Layer *parent);

    // Bounding box
    QRectF boundingRect() const;
    QPolygonF rotatedBBox() const;
    void invalidBBox();

    // Transform related
    const QTransform &transform() const;
    void applyTransform(const QTransform &transform);
    void setTransform(const QTransform &transform);
    void setTempTransform(const QTransform &transform);

    // Virtual functions
    virtual void calcBoundingBox() const;
    virtual shared_ptr<Shape> clone() const;
    virtual bool hitTest(QPointF global_coord, qreal tolerance) const;
    virtual bool hitTest(QRectF global_coord_rect) const;
    virtual void paint(QPainter *painter) const;
    virtual Type type() const;

  private:
    Layer *parent_;
    mutable bool bbox_need_recalc_;
    qreal rotation_;

  protected:
    QTransform temp_transform_;
    mutable QPolygonF rotated_bbox_;
    mutable QRectF bbox_;
    QTransform transform_;
};

typedef shared_ptr<Shape> ShapePtr;

#endif // SHAPE_H
