#include "svgpp_context.hpp"
#include <QPainter>
#include <QPainterPath>

SVGPPContext::SVGPPContext(QList<ShapePtr> &shapes): shapes_ { shapes } {
    transform = ublas::identity_matrix<double>(3, 3);
}

void SVGPPContext::path_move_to(double x, double y, tag::coordinate::absolute) {
    // qInfo() << "Move to " << x << "," << y;
    QPointF newPos = getTransformedPos(x, y);
    working_path_.moveTo(newPos.x(), newPos.y());
}

void SVGPPContext::path_line_to(double x, double y, tag::coordinate::absolute) {
    // qInfo() << "Line to " << x << "," << y;
    QPointF newPos = getTransformedPos(x, y);
    working_path_.lineTo(newPos.x(), newPos.y());
}

void SVGPPContext::path_cubic_bezier_to(
    double x1, double y1,
    double x2, double y2,
    double x, double y,
    tag::coordinate::absolute) {
    // qInfo() << "C bezier to " << x << "," << y;
    QPointF newPos2 = getTransformedPos(x2, y2);
    QPointF newPos1 = getTransformedPos(x1, y1);
    QPointF newPos = getTransformedPos(x, y);
    working_path_.cubicTo(newPos1.x(), newPos1.y(), newPos2.x(), newPos2.y(), newPos.x(), newPos.y());
}

void SVGPPContext::path_quadratic_bezier_to(
    double x1, double y1,
    double x, double y,
    tag::coordinate::absolute) {
    // qInfo() << "Q bezier to " << x << "," << y;
    QPointF newPos1 = getTransformedPos(x1, y1);
    QPointF newPos = getTransformedPos(x, y);
    working_path_.quadTo(newPos1.x(), newPos1.y(), newPos.x(), newPos.y());
}

void SVGPPContext::path_elliptical_arc_to(
    double rx, double ry, double x_axis_rotation,
    bool large_arc_flag, bool sweep_flag,
    double x2, double y2,
    tag::coordinate::absolute) {
    QPointF newPos2 = getTransformedPos(x2, y2);
    x2 = newPos2.x();
    y2 = newPos2.y();
    // qInfo() << "E bezier to " << x << "," << y;
    QPointF currentPos = working_path_.currentPosition();
    // TODO support rotated arc https://github.com/inkcut/inkcut/blob/ab27cf57ce5a5bd3bcaeef77bac28e4d6f92895a/inkcut/core/svg.py
    const double x1 = currentPos.x(),
                 y1 = currentPos.y(),
                 x1prime = (x1 - x2) / 2,
                 y1prime = (y1 - y2) / 2,
                 lamb = (x1prime * x1prime) / (rx * rx) + (y1prime * y1prime) / (ry * ry);

    if (lamb >= 1) {
        ry = sqrt(lamb) * ry;
        rx = sqrt(lamb) * rx;
    }

    // Back to https://www.w3.org/TR/SVG/implnote.html F.6.5
    double radicand = (rx * rx * ry * ry - rx * rx * y1prime * y1prime - ry * ry * x1prime * x1prime);
    radicand /= (rx * rx * y1prime * y1prime + ry * ry * x1prime * x1prime);

    if (radicand < 0) {
        radicand = 0;
    }

    const double factor = (large_arc_flag == sweep_flag ? -1 : 1) * sqrt(radicand);
    const double cxprime = factor * rx * y1prime / ry,
                 cyprime = -factor * ry * x1prime / rx,
                 cx = cxprime + (x1 + x2) / 2,
                 cy = cyprime + (y1 + y2) / 2,
                 start_theta = -atan2((y1 - cy) * rx, (x1 - cx) * ry),
                 start_phi = -atan2(y1 - cy, x1 - cx),
                 end_phi = -atan2(y2 - cy, x2 - cx);
    double sweep_length = end_phi - start_phi;

    if (sweep_length < 0 && !sweep_flag) {
        sweep_length += 2 * 3.1415926;
    }

    if (sweep_length > 0 && sweep_flag) {
        sweep_length -= 2 * 3.1415926;
    }

    working_path_.arcTo(cx - rx, cy - ry, rx * 2, ry * 2,
                        start_theta * 360 / 2 / 3.1415926, sweep_length * 360 / 2 / 3.1415926);
}

void SVGPPContext::path_close_subpath() {
    working_path_.closeSubpath();
}

void SVGPPContext::path_exit() {
    ShapePtr shape(new PathShape(working_path_));
    shape->simplify();
    shapes_.push_back(shape);
    transform = ublas::identity_matrix<double>(3, 3);
    working_path_ = QPainterPath();
}

QPointF SVGPPContext::getTransformedPos(double x, double y) {
    matrix_t m = ublas::identity_matrix<double>(3, 1);
    m <<= x, y, 1;
    matrix_t npos = ublas::prod(transform, m);
    return QPointF(npos(0, 0), npos(1, 0));
}

void SVGPPContext::transform_matrix(const boost::array<double, 6> &matrix) {
    //qInfo() << "Load matrix " << matrix[0] << matrix[1] << matrix[2] << matrix[3] << matrix[4] << matrix[5];
    matrix_t m(3, 3);
    m <<=
        matrix[0], matrix[2], matrix[4],
        matrix[1], matrix[3], matrix[5],
        0, 0, 1;
    // todo fix logic
    transform = m; //ublas::prod(transform, m);
}

void SVGPPContext::transform_translate(double tx, double ty) {
    matrix_t m = ublas::identity_matrix<double>(3, 3);
    m(0, 2) = tx;
    m(1, 2) = ty;
    transform = ublas::prod(transform, m);
    //qInfo() << "Transform translate";
}

void SVGPPContext::transform_translate(double tx) {
    transform_translate(tx, tx);
    //qInfo() << "Transform translate";
}

void SVGPPContext::transform_scale(double sx, double sy) {
    matrix_t m = ublas::identity_matrix<double>(3, 3);
    m(0, 0) = sx;
    m(1, 1) = sy;
    transform = ublas::prod(transform, m);
    //qInfo() << "Transform scale";
}

void SVGPPContext::transform_scale(double scale) {
    transform_scale(scale, scale);
    //qInfo() << "Transform scale";
}

void SVGPPContext::transform_rotate(double angle) {
    angle *= boost::math::constants::degree<double>();
    matrix_t m(3, 3);
    m <<=
        std::cos(angle), -std::sin(angle), 0,
        std::sin(angle),  std::cos(angle), 0,
        0, 0, 1;
    transform = ublas::prod(transform, m);
    qInfo() << "Transform rotate";
}
