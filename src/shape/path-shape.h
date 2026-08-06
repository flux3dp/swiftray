#pragma once

#include <shape/shape.h>
#include <shape/stl-placement.h>

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

  const StlPlacement &stlPlacement() const { return stl_placement_; }

  void setStlPlacement(const StlPlacement &placement) { stl_placement_ = placement; }

  bool isStlPlaceholder() const { return stl_placement_.isValid(); }

  friend class DocumentSerializer;

private:
  void calcBoundingBox() const override;

  mutable QRectF hit_test_rect_;

protected:
  QPainterPath path_;
  // Note: DocumentSerializer (bvg save / load) does not keep this. Add it for debug if needed.
  StlPlacement stl_placement_;
};
