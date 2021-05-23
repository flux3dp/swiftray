#ifndef VCANVAS_H
#define VCANVAS_H
#include <QtQuick>
#include "shape/shape.hpp"
#include "parser/svgpp_parser.hpp"

class VCanvas : public QQuickPaintedItem {
        Q_OBJECT
        QML_ELEMENT

    public:
        VCanvas(QQuickItem *parent = 0);
        void paint(QPainter *painter);
        void loop();
        void loadSvg(QByteArray &data);


        void mousePressEvent(QMouseEvent *e) override;
        void mouseMoveEvent(QMouseEvent *e) override;
        void mouseReleaseEvent(QMouseEvent *e) override;
        void wheelEvent(QWheelEvent *e) override;
        bool event(QEvent *e) override;

    private:
        bool rightAligned;
        bool ready;
        float scrollX;
        float scrollY;
        float scale;
        int counter;
        SVGPPParser svgppParser;


        QTimer *timer;
        bool m_smallScreen;
        QPoint m_mousePress;
        bool m_mouseDrag;

        QHash<int, int> m_fingerPointMapping;

    signals:
        void rightAlignedChanged();
};

#endif // VCANVAS_H
