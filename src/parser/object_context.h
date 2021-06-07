#include <parser/svgpp_common.h>
#pragma once

class ObjectContext {
public:
  void set(tag::attribute::width, double val){};
  void set(tag::attribute::height, double val){};
  void set(tag::attribute::preserveAspectRatio, bool, tag::value::none){};

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

  template <typename MinMax, typename SliceMeet>
  void set(tag::attribute::preserveAspectRatio, bool, MinMax, SliceMeet){};

  template <class IRI>
  void set(svgpp::tag::attribute::data_original_layer, tag::iri_fragment,
           IRI const &fragment) {
    qInfo() << "xlink::href"
            << QString::fromStdString(
                   std::string(fragment.begin(), fragment.end()));
  }

  void set(svgpp::tag::attribute::data_original_layer, RangedChar fragment) {
    qInfo() << "xlink::href"
            << QString::fromStdString(
                   std::string(fragment.begin(), fragment.end()));
  }

private:
  double empty;
};
