#ifndef PATHSHAPE_H
#define PATHSHAPE_H

#include <shape/shape.hpp>

using namespace std;

class PathShape : public Shape {
    public:
        PathShape(QPainterPath path);
        virtual ~PathShape();
        void simplify() override;
        void cacheSelectionTestingData() override;
        bool testHit(QPointF global_coord, qreal tolerance) const override;
        bool testHit(QRectF global_coord_rect) const override;
        QRectF boundingRect() const override;
        void paint(QPainter *painter) const override;
        shared_ptr<Shape> clone() const override;
        QPainterPath path_;
    private:
        QList<QPointF> selection_testing_points_;
        QRectF selection_testing_rect_;
};

#endif // PATHSHAPE_H
