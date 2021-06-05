#include <QPainter>
#include <QPixmap>
#include <shape/shape.h>

#ifndef BITMAPSHAPE_H
#define BITMAPSHAPE_H


class BitmapShape : public Shape {
    public:
        BitmapShape(QImage &image);
        bool testHit(QPointF global_coord, qreal tolerance) const override;
        bool testHit(QRectF global_coord_rect) const override;
        QRectF boundingRect() const override;
        void paint(QPainter *painter) const override;
        ShapePtr clone() const override;
        Shape::Type type() const override;
        QPixmap bitmap_;
    private:
        QRectF bounding_rect_;
};

#endif // BITMAPSHAPE_H
