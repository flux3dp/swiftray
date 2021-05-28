#include <parser/svgpp_common.hpp>
#ifndef SVGPPCONTEXT_H
#define SVGPPCONTEXT_H
#include <svgpp/svgpp.hpp>
#include <QDebug>
#include <QPainter>
#include <string>
#include <shape/shape.hpp>
#include <shape/path_shape.h>
#include <container/shape_collection.h>

#include <boost/math/constants/constants.hpp>
#include <boost/numeric/ublas/assignment.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/io.hpp>
namespace ublas = boost::numeric::ublas;
typedef ublas::matrix<double> matrix_t;

using namespace svgpp;

class SVGPPContext {
    public:
        SVGPPContext(ShapeCollection &shapes);

        void on_enter_element(tag::element::any)
        {}

        void on_exit_element()
        {}

        void transform_matrix(const boost::array<double, 6> &matrix);

        void transform_translate(double tx, double ty);

        void transform_translate(double tx);

        void transform_scale(double sx, double sy);

        void transform_scale(double scale);

        void transform_rotate(double angle);

        void transform_rotate(double angle, double cx, double cy);

        void transform_skew_x(double angle);

        void transform_skew_y(double angle);

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

        static bool unknown_attribute_error(std::string name) {
            qInfo() << "Unknown attribute" << QString::fromStdString(name);
            return true;
        }

        QPointF getTransformedPos(double x, double y);

    private:
        ShapeCollection &shapes_;
        QPainterPath working_path_;
        matrix_t transform;
};

#endif // SVGPPCONTEXT_H
