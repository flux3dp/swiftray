
#include <parser/contexts/base_context.h>

#pragma once

class ImageContext : public BaseContext {
public:
  ImageContext(BaseContext const &parent) : BaseContext(parent) {
    qInfo() << "Enter image";
  }

  boost::optional<double> const &width() const { return width_; }
  boost::optional<double> const &height() const { return height_; }

  using BaseContext::set;

  template <class IRI>
  void set(tag::attribute::xlink::href, tag::iri_fragment,
           IRI const &fragment) {
    qInfo() << "xlink::href" << fragment;
  }

  void set(tag::attribute::xlink::href, RangedChar fragment) {
    qInfo() << "xlink::href"
            << QString::fromStdString(
                   std::string(fragment.begin(), fragment.end()));
  }

  void set(tag::attribute::x, double val) { x_ = val; }

  void set(tag::attribute::y, double val) { y_ = val; }

  void set(tag::attribute::width, double val) { width_ = val; }

  void set(tag::attribute::height, double val) { height_ = val; }

  void on_exit_element();

private:
  std::string fragment_id_;
  double x_, y_;
  boost::optional<double> width_, height_;
};
