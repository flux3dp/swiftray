#ifndef GROUPSHAPE_H
#define GROUPSHAPE_H

#include <shape/shape.h>

class GroupShape : public Shape {
    public:
        GroupShape();
        GroupShape(QList<ShapePtr> &children);
        bool hitTest(QPointF global_coord, qreal tolerance) override;
        bool hitTest(QRectF global_coord_rect) override;
        void calcBoundingBox() override;
        void paint(QPainter *painter) override;
        ShapePtr clone() const override;
        Shape::Type type() const override;
        QList<ShapePtr> &children() ;
    private:
        QList<ShapePtr> children_;
};

#endif // GROUPSHAPE_H
