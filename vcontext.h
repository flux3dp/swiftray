#ifndef VCONTEXT_H
#define VCONTEXT_H
#include <svgpp/svgpp.hpp>
#include <QDebug>
#include <QPainter>

using namespace svgpp;

class VContext {
    public:
        void on_enter_element(tag::element::any)
        {}

        void on_exit_element()
        {}

        void path_move_to(double x, double y, tag::coordinate::absolute);

        void path_line_to(double x, double y, tag::coordinate::absolute);

        void path_cubic_bezier_to(
            double x1, double y1,
            double x2, double y2,
            double x, double y,
            tag::coordinate::absolute);

        void path_quadratic_bezier_to(
            double x1, double y1,
            double x, double y,
            tag::coordinate::absolute);

        void path_elliptical_arc_to(
            double rx, double ry, double x_axis_rotation,
            bool large_arc_flag, bool sweep_flag,
            double x, double y,
            tag::coordinate::absolute);

        void path_close_subpath();

        void path_exit();

        void setPainter(QPainter *painter) {
            m_painter = painter;
        }

        QPainter *painter() {
            return m_painter;
        }
    private:
        QPainter *m_painter;
        QList<QPointF> m_points;
};

#endif // VCONTEXT_H
