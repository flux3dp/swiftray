#ifndef GROUPSHAPE_H
#define GROUPSHAPE_H

#include <shape/shape.h>

class GroupShape : public Shape {
    public:
        GroupShape();
        GroupShape(QList<ShapePtr> &children);
        void simplify() override;
        void cacheSelectionTestingData() override;
        bool testHit(QPointF global_coord, qreal tolerance) const override;
        bool testHit(QRectF global_coord_rect) const override;
        QRectF boundingRect() const override;
        void paint(QPainter *painter) const override;
        ShapePtr clone() const override;
        QList<ShapePtr> &children() ;
    private:
        QList<ShapePtr> children_;
        QRectF bounding_rect_;
};

#endif // GROUPSHAPE_H
