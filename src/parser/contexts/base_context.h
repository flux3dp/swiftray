
#include <parser/contexts/stylable_context.h>
#include <parser/contexts/object_context.h>
#include <parser/contexts/transformable_context.h>
#include <parser/contexts/svgpp_doc.h>

#pragma once


class BaseContext : public StylableContext, public ObjectContext, public TransformableContext {
public:
  BaseContext(SVGPPDoc &document, double resolutionDPI): doc_(document) {
    qInfo() << "<base>";
    length_factory_.set_absolute_units_coefficient(resolutionDPI,
                                                   tag::length_units::in());
  }

  using ObjectContext::set;
  using StylableContext::set;


  template <class Range>
  void set_text(Range const &text) {
    std::string str;
    str.append(boost::begin(text), boost::end(text));
    if (svgpp_active_layer_ != nullptr) {
      svgpp_active_layer_->setName(QString::fromStdString(str));
    }
  }

  // Called by Context Factory
  void on_exit_element() {
    qInfo() << "</base>";
  }

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

  SVGPPDoc & document() const { return doc_; }
private:
  length_factory_type length_factory_;
  SVGPPDoc & doc_;
};
