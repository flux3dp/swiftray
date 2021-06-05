#include <QPainter>
#include <QPixmap>
#include <shape/shape.h>

#ifndef BITMAPSHAPE_H
#define BITMAPSHAPE_H


class BitmapShape : public Shape {
    public:
        BitmapShape(QImage &image);
        BitmapShape(const BitmapShape &orig);
        bool hitTest(QPointF global_coord, qreal tolerance) const override;
        bool hitTest(QRectF global_coord_rect) const override;
        QRectF boundingRect() const override;
        void paint(QPainter *painter) override;
        ShapePtr clone() const override;
        Shape::Type type() const override;
        QImage &image();
    private:
        QRectF bounding_rect_;
        unique_ptr<QPixmap> bitmap_;
        QImage tinted_image_;
        std::uintptr_t tinted_signature;
};

#endif // BITMAPSHAPE_H
