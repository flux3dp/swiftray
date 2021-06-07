#include <QDebug>
#include <parser/base_context.h>

#pragma once

class UseContext : public BaseContext {
public:
  UseContext(BaseContext const &parent) : BaseContext(parent) {
    qInfo() << "Enter use";
  }

  boost::optional<double> const &width() const { return width_; }
  boost::optional<double> const &height() const { return height_; }

  using BaseContext::set;

  template <class IRI>
  void set(tag::attribute::xlink::href, tag::iri_fragment,
           IRI const &fragment) {
    fragment_id_.assign(boost::begin(fragment), boost::end(fragment));
  }

  template <class IRI>
  void set(tag::attribute::xlink::href, IRI const &fragment) {
    std::cerr << "External references aren't supported\n";
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