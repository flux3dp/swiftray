#ifndef PATHSHAPE_H
#define PATHSHAPE_H

#include <shape/shape.h>

using namespace std;

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
        bool hitTest(QPointF global_coord, qreal tolerance) override;
        bool hitTest(QRectF global_coord_rect) override;
        void calcBoundingBox() override;
        void paint(QPainter *painter) override;
        shared_ptr<Shape> clone() const override;
        Shape::Type type() const override;
        QPainterPath& path();
        void setPath(QPainterPath &path);
        QPainterPath path_;
    private:
        QHash<int, NodeType> node_types_;
        QRectF hit_test_rect_;
};

#endif // PATHSHAPE_H
