#include <QPainter>
#include <QPainterPath>

#ifndef SHAPE_H
#define SHAPE_H

using namespace std;
class Layer;
class Shape {
    public:
        enum class Type {
            None,
            Path,
            Bitmap,
            Text,
            Group
        };
        Shape() noexcept;
        virtual ~Shape();
        // Common members
        bool selected;
        QPointF pos() const;
        qreal x() const;
        qreal y() const;
        qreal rotation() const;
        qreal scaleX() const;
        qreal scaleY() const;
        Layer* parent() const;
        void setParent(Layer* parent);
        void setX(qreal x);
        void setY(qreal y);
        void setPos(QPointF pos);
        void setRotation(qreal r);
        QTransform transform() const;
        QRectF boundingRect();
        void applyTransform(QTransform transform);
        void setTransform(QTransform transform);

        // Virtual functions
        virtual bool hitTest(QPointF global_coord, qreal tolerance);
        virtual bool hitTest(QRectF global_coord_rect);
        virtual void paint(QPainter *painter);
        virtual shared_ptr<Shape> clone() const;
        virtual Type type() const;
        virtual void calcBoundingBox();
        QTransform temp_transform_;
        QRectF unrotated_bbox_;
    private:
        Layer* parent_;
        QTransform transform_;
        bool bbox_need_recalc_;
        qreal rotation_;
    protected:
        QRectF bbox_;
};

typedef shared_ptr<Shape> ShapePtr;

#endif //SHAPE_H
