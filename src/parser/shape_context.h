#include <QPainterPath>
#include <parser/base_context.h>

#include <boost/math/constants/constants.hpp>
#include <boost/numeric/ublas/assignment.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/matrix.hpp>

#pragma once

namespace ublas = boost::numeric::ublas;
typedef ublas::matrix<double> matrix_t;

class ShapeContext : public BaseContext {
public:
  ShapeContext(BaseContext const &parent) : BaseContext(parent) {
    qInfo() << "Enter shape";
    this->transform_ = ublas::identity_matrix<double>(3, 3);
  }

  using BaseContext::set;
  using StylableContext::set;
  // Path Events Policy methods
  void path_move_to(double x, double y, tag::coordinate::absolute) {
    QPointF newPos = getTransformedPos(x, y);
    working_path_.moveTo(newPos.x(), newPos.y());
  }

  void path_line_to(double x, double y, tag::coordinate::absolute) {
    QPointF newPos = getTransformedPos(x, y);
    working_path_.lineTo(newPos.x(), newPos.y());
  }

  void path_cubic_bezier_to(double x1, double y1, double x2, double y2,
                            double x, double y, tag::coordinate::absolute) {

    QPointF newPos2 = getTransformedPos(x2, y2);
    QPointF newPos1 = getTransformedPos(x1, y1);
    QPointF newPos = getTransformedPos(x, y);
    working_path_.cubicTo(newPos1.x(), newPos1.y(), newPos2.x(), newPos2.y(),
                          newPos.x(), newPos.y());
  }

  void path_quadratic_bezier_to(double x1, double y1, double x, double y,
                                tag::coordinate::absolute) {

    QPointF newPos1 = getTransformedPos(x1, y1);
    QPointF newPos = getTransformedPos(x, y);
    working_path_.quadTo(newPos1.x(), newPos1.y(), newPos.x(), newPos.y());
  }

  void path_elliptical_arc_to(double rx, double ry, double x_axis_rotation,
                              bool large_arc_flag, bool sweep_flag, double x2,
                              double y2, tag::coordinate::absolute) {
    QPointF newPos2 = getTransformedPos(x2, y2);
    x2 = newPos2.x();
    y2 = newPos2.y();
    // qInfo() << "E bezier to " << x << "," << y;
    QPointF currentPos = working_path_.currentPosition();
    // TODO support rotated arc
    // https://github.com/inkcut/inkcut/blob/ab27cf57ce5a5bd3bcaeef77bac28e4d6f92895a/inkcut/core/svg.py
    const double x1 = currentPos.x(), y1 = currentPos.y(),
                 x1prime = (x1 - x2) / 2, y1prime = (y1 - y2) / 2,
                 lamb = (x1prime * x1prime) / (rx * rx) +
                        (y1prime * y1prime) / (ry * ry);

    if (lamb >= 1) {
      ry = sqrt(lamb) * ry;
      rx = sqrt(lamb) * rx;
    }

    // Back to https://www.w3.org/TR/SVG/implnote.html F.6.5
    double radicand = (rx * rx * ry * ry - rx * rx * y1prime * y1prime -
                       ry * ry * x1prime * x1prime);
    radicand /= (rx * rx * y1prime * y1prime + ry * ry * x1prime * x1prime);

    if (radicand < 0) {
      radicand = 0;
    }

    const double factor =
        (large_arc_flag == sweep_flag ? -1 : 1) * sqrt(radicand);
    const double cxprime = factor * rx * y1prime / ry,
                 cyprime = -factor * ry * x1prime / rx,
                 cx = cxprime + (x1 + x2) / 2, cy = cyprime + (y1 + y2) / 2,
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
                        start_theta * 360 / 2 / 3.1415926,
                        sweep_length * 360 / 2 / 3.1415926);
  }

  void path_close_subpath() { 
    working_path_.closeSubpath(); 
  }

  void path_exit() {
    ShapePtr shape = make_shared<PathShape>(working_path_);
    svgpp_shapes->push_back(shape);
    this->transform_ = ublas::identity_matrix<double>(3, 3);
    working_path_ = QPainterPath();
  }

  // Marker Events Policy method
  void marker(marker_vertex v, double x, double y, double directionality,
              unsigned marker_index) {
    if (marker_index >= markers_.size())
      markers_.resize(marker_index + 1);
    MarkerPos &m = markers_[marker_index];
    m.v = v;
    m.x = x;
    m.y = y;
    m.directionality = directionality;
  }

private:
  struct MarkerPos {
    marker_vertex v;
    double x, y, directionality;
  };

  typedef std::vector<MarkerPos> Markers;
  Markers markers_;

  QPainterPath working_path_;
};