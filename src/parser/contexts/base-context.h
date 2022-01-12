
#include <parser/contexts/stylable-context.h>
#include <parser/contexts/object-context.h>
#include <parser/contexts/transformable-context.h>
#include <parser/contexts/svgpp-doc.h>

#pragma once
namespace Parser {

class BaseContext : public StylableContext, public ObjectContext, public TransformableContext {
public:
  BaseContext(SVGPPDoc &svgpp_doc, double resolutionDPI) : doc_(svgpp_doc) {
    qInfo() << "<base>";
    id_ = "";
    class_name_ = "";
    length_factory_.set_absolute_units_coefficient(resolutionDPI,
                                                   tag::length_units::in());
  }

  using ObjectContext::set;
  using StylableContext::set;


  template<class Range>
  void set_text(Range const &text) {
    std::string str;
    str.append(boost::begin(text), boost::end(text));
    if (svgpp_active_layer_ != nullptr) {
      qInfo() << "[SVGPP] Set Active Layer's Name to " << QString::fromStdString(str);
      svgpp_active_layer_->setName(QString::fromStdString(str));
    }
  }

  void set(tag::attribute::id, RangedChar id) {
    std::string str;
    str.append(boost::begin(id), boost::end(id));
    id_ = QString::fromStdString(str);
  }

  void set(tag::attribute::class_, RangedChar class_) {
    std::string str;
    str.append(boost::begin(class_), boost::end(class_));
    class_name_ = QString::fromStdString(str);
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
    qInfo() << "[SVGPP] Unknown attribute" << QString::fromStdString(name);
    return true;
  }

  // Length Policy interface
  typedef factory::length::unitless<> length_factory_type;

  length_factory_type const &length_factory() const { return length_factory_; }

  SVGPPDoc &svgppDoc() const { return doc_; }

  virtual std::string type() {
    return "svg";
  }

  QString id_;
  QString class_name_;

private:
  length_factory_type length_factory_;
  SVGPPDoc &doc_;

};

}
