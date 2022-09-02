#pragma once

#include <shape/shape.h>

//using namespace std;

class PathShape : public Shape {
public:
  enum class NodeType {
    CurveSymmetry,
    CurveSmooth,
    CurveCorner,
    CurveCtrlPrev,
    CurveCtrlNext,
    LINE_TO,
    MOVE_TO
  };

  PathShape() noexcept;

  PathShape(QPainterPath path);

  virtual ~PathShape();

  ShapePtr clone() const override;

  bool hitTest(QPointF global_coord, qreal tolerance) const override;

  bool hitTest(QRectF global_coord_rect) const override;

  void paint(QPainter *painter) const override;

  Shape::Type type() const override;

  const QPainterPath &path() const;

  void setPath(const QPainterPath &path);

  bool isFilled() const;

  void setFilled(bool filled);

  friend class DocumentSerializer;

private:
  void calcBoundingBox() const override;

  bool filled_;
  mutable QRectF hit_test_rect_;

protected:
  QPainterPath path_;
};
