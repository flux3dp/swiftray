#include <shape/shape.h>

#ifndef LAYER_H
#define LAYER_H

class Layer {
    public:
        Layer();
        void paint(QPainter *painter, int counter) const;
        void addShape(ShapePtr shape);
        void removeShape(ShapePtr shape);
        void clear();
        shared_ptr<Layer> clone();
        QList<ShapePtr> &children();

        QColor color();
        void setColor(QColor color);

        QString name;
    private:
        QColor color_;
        QList<ShapePtr> children_;
};

typedef shared_ptr<Layer> LayerPtr;

#endif // LAYER_H
