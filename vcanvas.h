#ifndef VCANVAS_H
#define VCANVAS_H
#include <QtQuick>
#include <shape/shape.hpp>
#include <parser/svgpp_parser.hpp>
#include <canvas/canvas_data.hpp>
#include <canvas/transform_box.hpp>

class VCanvas : public QQuickPaintedItem {
        Q_OBJECT
        QML_ELEMENT

    public:
        VCanvas(QQuickItem *parent = 0);
        void paint(QPainter *painter) override;
        void loop();
        void loadSvg(QByteArray &data);


        void mousePressEvent(QMouseEvent *e) override;
        void mouseMoveEvent(QMouseEvent *e) override;
        void mouseReleaseEvent(QMouseEvent *e) override;
        void wheelEvent(QWheelEvent *e) override;
        bool event(QEvent *e) override;

    private:
        bool ready;
        int counter;
        CanvasData canvas_data;
        SVGPPParser svgpp_parser;
        QList<Shape> shapes;
        TransformBox transform_box;

        QTimer *timer;
        bool small_screen;
        QPoint mouse_press;
        bool mouse_drag;
        QRectF selection_box;
        QPointF selection_start;
        QHash<int, int> m_fingerPointMapping;

    signals:
        void rightAlignedChanged();
};

#endif // VCANVAS_H
