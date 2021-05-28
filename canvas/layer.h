#ifndef LAYER_H
#define LAYER_H

#include <shape/shape.hpp>
#include <canvas/canvas_data.hpp>

class Layer {
    public:
        Layer(CanvasData &canvas);
        void paint(QPainter *painter);
        void addShape(Shape shape);
        void removeShape(ShapePtr shape);
        void clear();

        void color();
        void setColor(QColor color);
    private:
        QColor color_;
        CanvasData &canvas_;
        QList<ShapePtr> shapes_;
};

#endif // LAYER_H
