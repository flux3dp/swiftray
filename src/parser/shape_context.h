#include <parser/base_context.h>

#pragma once


class ShapeContext : public BaseContext {
public:
  ShapeContext(BaseContext const &parent) : BaseContext(parent) {
    qInfo() << "Enter shape";
  }

  using BaseContext::set;
  using StylableContext::set;
  // Path Events Policy methods
  void path_move_to(double x, double y, tag::coordinate::absolute) {
    std::cout << "Move to " << x << "," << y << std::endl;
  }

  void path_line_to(double x, double y, tag::coordinate::absolute) {
    // std::cout << "Line to " << x << "," << y << std::endl;
  }

  void path_cubic_bezier_to(double x1, double y1, double x2, double y2,
                            double x, double y, tag::coordinate::absolute) {}

  void path_quadratic_bezier_to(double x1, double y1, double x, double y,
                                tag::coordinate::absolute) {}

  void path_elliptical_arc_to(double rx, double ry, double x_axis_rotation,
                              bool large_arc_flag, bool sweep_flag, double x,
                              double y, tag::coordinate::absolute) {}

  void path_close_subpath() {}

  void path_exit() {}

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
};