#include "vcontext.h"

void VContext::path_move_to(double x, double y, tag::coordinate::absolute) {
    // qInfo() << "Move to " << x << "," << y;
    m_points.clear();
    m_points << QPointF(x, y);
}

void VContext::path_line_to(double x, double y, tag::coordinate::absolute) {
    // qInfo() << "Line to " << x << "," << y;
    m_points << QPointF(x, y);
}

void VContext::path_cubic_bezier_to(
    double x1, double y1,
    double x2, double y2,
    double x, double y,
    tag::coordinate::absolute) {
    // qInfo() << "C bezier to " << x << "," << y;
    m_points << QPointF(x, y);
}

void VContext::path_quadratic_bezier_to(
    double x1, double y1,
    double x, double y,
    tag::coordinate::absolute) {
    // qInfo() << "Q bezier to " << x << "," << y;
    m_points << QPointF(x, y);
}

void VContext::path_elliptical_arc_to(
    double rx, double ry, double x_axis_rotation,
    bool large_arc_flag, bool sweep_flag,
    double x, double y,
    tag::coordinate::absolute) {
    // qInfo() << "E bezier to " << x << "," << y;
    m_points << QPointF(x, y);
}

void VContext::path_close_subpath() {
    m_painter->drawPolyline(m_points);
}

void VContext::path_exit() {
    m_painter->drawPolyline(m_points);
}
