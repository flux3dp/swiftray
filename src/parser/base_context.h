
#include <parser/stylable_context.h>
#include <parser/object_context.h>
#include <parser/transformable_context.h>

#pragma once


class BaseContext : public StylableContext, public ObjectContext, public TransformableContext {
public:
  BaseContext(double resolutionDPI) {
    qInfo() << "<base>";
    length_factory_.set_absolute_units_coefficient(resolutionDPI,
                                                   tag::length_units::in());
  }

  using ObjectContext::set;
  using StylableContext::set;

  // Called by Context Factory
  void on_exit_element() { qInfo() << "</>"; }

  // Transform Events Policy
  void transform_matrix(const boost::array<double, 6> &matrix) {}

  // Viewport Events Policy
  void set_viewport(double viewport_x, double viewport_y, double viewport_width,
                    double viewport_height) {
    length_factory_.set_viewport_size(viewport_width, viewport_height);
  }

  void set_viewbox_size(double viewbox_width, double viewbox_height) {
    length_factory_.set_viewport_size(viewbox_width, viewbox_height);
  }

  void disable_rendering() {}

  static bool unknown_attribute_error(std::string name) {
    qInfo() << "Unknown attribute" << QString::fromStdString(name);
    return true;
  }

  // Length Policy interface
  typedef factory::length::unitless<> length_factory_type;

  length_factory_type const &length_factory() const { return length_factory_; }

private:
  length_factory_type length_factory_;
};
