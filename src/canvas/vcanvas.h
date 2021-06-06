#ifndef VCANVAS_H
#define VCANVAS_H
#include <QtQuick>
#include <shape/shape.h>
#include <parser/svgpp_parser.h>
#include <canvas/scene.h>
#include <canvas/controls/transform_box.h>
#include <canvas/controls/multi_selection_box.h>
#include <canvas/controls/rect_drawer.h>
#include <canvas/controls/oval_drawer.h>
#include <canvas/controls/line_drawer.h>
#include <canvas/controls/path_drawer.h>
#include <canvas/controls/path_editor.h>
#include <canvas/controls/text_drawer.h>
#include <canvas/controls/grid.h>
#include <canvas/controls/canvas_control.h>

class VCanvas : public QQuickPaintedItem {
        Q_OBJECT
        QML_ELEMENT

    public:
        VCanvas(QQuickItem *parent = 0);
        void paint(QPainter *painter) override;
        void loop();
        void loadSVG(QByteArray &data);
        void keyPressEvent(QKeyEvent *e) override;
        void mousePressEvent(QMouseEvent *e) override;
        void mouseMoveEvent(QMouseEvent *e) override;
        void mouseReleaseEvent(QMouseEvent *e) override;
        void mouseDoubleClickEvent(QMouseEvent *e) override;
        void wheelEvent(QWheelEvent *e) override;
        bool event(QEvent *e) override;
        Scene &scene();

    public Q_SLOTS:
        void editCut();
        void editCopy();
        void editPaste();
        void editDelete();
        void editUndo();
        void editRedo();
        void editSelectAll();
        void editGroup();
        void editUngroup();
        void editDrawRect();
        void editDrawOval();
        void editDrawLine();
        void editDrawPath();
        void editDrawText();

        void editUnion();
        void editSubtract();
        void editIntersect();
        void editDifference();

        void importImage(QImage &image);

        void fitWindow();
    private:
        bool ready;
        int counter;
        Scene scene_;
        SVGPPParser svgpp_parser;
        TransformBox transform_box_;
        MultiSelectionBox multi_selection_box_;
        Grid grid_;
        LineDrawer line_drawer_;
        OvalDrawer oval_drawer_;
        PathDrawer path_drawer_;
        PathEditor path_editor_;
        RectDrawer rect_drawer_;
        TextDrawer text_drawer_;
        QList<CanvasControl *> controls_;

        QTimer *timer;
        QHash<int, int> m_fingerPointMapping;


    signals:
        void rightAlignedChanged();
};

#endif // VCANVAS_H
