#ifndef PATHSHAPE_H
#define PATHSHAPE_H

#include <shape/shape.h>

using namespace std;

namespace Controls { class PathEdit; }
class PathShape : public Shape {
  public:
    enum class NodeType {
        CURVE_SYMMETRY,
        CURVE_SMOOTH,
        CURVE_CORNER,
        CURVE_CTRL_PREV,
        CURVE_CTRL_NEXT,
        LINE_TO,
        MOVE_TO
    };
    PathShape() noexcept;
    PathShape(QPainterPath path);
    virtual ~PathShape();

    void calcBoundingBox() const override;
    ShapePtr clone() const override;
    bool hitTest(QPointF global_coord, qreal tolerance) const override;
    bool hitTest(QRectF global_coord_rect) const override;
    void paint(QPainter *painter) const override;
    Shape::Type type() const override;

    const QPainterPath &path() const;
    void setPath(QPainterPath &path);

    friend class Controls::PathEdit;

  private:
    QHash<int, NodeType> node_types_;
    mutable QRectF hit_test_rect_;

  protected:
    QPainterPath path_;
};

#endif // PATHSHAPE_H
