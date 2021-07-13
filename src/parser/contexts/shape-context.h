#include <QPainterPath>
#include <QString>
#include <QTransform>
#include <parser/contexts/base-context.h>

#include <boost/math/constants/constants.hpp>
#include <boost/numeric/ublas/assignment.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/matrix.hpp>

#pragma once

namespace Parser {

namespace ublas = boost::numeric::ublas;
typedef ublas::matrix<double> matrix_t;

class ShapeContext : public BaseContext {
public:
  ShapeContext(BaseContext const &parent) : BaseContext(parent) {
    //qInfo() << "<shape>";
    this->transform_ = ublas::identity_matrix<double>(3, 3);
    working_path_ = QPainterPath();
  }

  void check_style() {
    if (this->id_.isEmpty() && this->class_name_.isEmpty()) return;
    SVGStyleSelector::NodePtr node;
    SVGNode mockup("path", this->id_, this->class_name_);
    node.ptr = &mockup;
    // TODO(Rewrite declaration for node to process only simple rules)
    auto decls = svgpp_style_selector->declarationsForNode(node);
    for (auto &decl : decls) {
      if (decl.d->property.isEmpty())
        continue;
      if (decl.d->property == "fill") {
        if (decl.d->values.size() > 0) {
          QColor c(decl.d->values[0].toString());
          style().fill_paint_ = color_t(c.red(), c.green(), c.blue());
        }
      } else if (decl.d->property == "stroke") {
        if (decl.d->values.size() > 0) {
          QColor c(decl.d->values[0].toString());
          style().stroke_paint_ = color_t(c.red(), c.green(), c.blue());
        }
      }
    }
  }

  void on_exit_element() {
    check_style();
    QPainterPath mapped_path = qtransform().map(working_path_);
    ShapePtr shape = make_shared<PathShape>(mapped_path);
    QString layer_name = this->strokeColor() == "N/A" ? this->fillColor() : this->strokeColor();
    if (this->strokeColor() == "N/A" && this->fillColor() != "N/A") ((PathShape *) shape.get())->setFilled(true);
    svgpp_add_shape(shape, layer_name);
  }

  // Path Events Policy methods
  void path_move_to(double x, double y, tag::coordinate::absolute) {
    working_path_.moveTo(x, y);
  }

  void path_line_to(double x, double y, tag::coordinate::absolute) {
    working_path_.lineTo(x, y);
  }

  void path_cubic_bezier_to(double x1, double y1, double x2, double y2,
                            double x, double y, tag::coordinate::absolute) {
    working_path_.cubicTo(x1, y1, x2, y2,
                          x, y);
  }

  void path_quadratic_bezier_to(double x1, double y1, double x, double y,
                                tag::coordinate::absolute) {
    working_path_.quadTo(x1, y1, x, y);
  }

  void path_elliptical_arc_to(double rx, double ry, double x_axis_rotation,
                              bool large_arc_flag, bool sweep_flag, double x2,
                              double y2, tag::coordinate::absolute) {
    const QPointF &currentPos = working_path_.currentPosition();
    // TODO (Support rotated arc)
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
  }

  // Marker Events Policy method
  // TODO (Truly support marker)
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

  string type() {
    return "path";
  }

  using BaseContext::set;
  using StylableContext::set;

private:
  struct MarkerPos {
    marker_vertex v;
    double x, y, directionality;
  };

  typedef std::vector<MarkerPos> Markers;
  Markers markers_;

  QPainterPath working_path_;
};

}