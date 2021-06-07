#include <QDebug>
#include <parser/contexts/base_context.h>
#include <iostream>

#pragma once

class GroupContext : public BaseContext {
public:
  GroupContext(BaseContext const &parent) : BaseContext(parent) {
    qInfo() << "Enter group";
  }

  void set(svgpp::tag::attribute::data_strength, double val) {
    qInfo() << "Data-Strength" << val;
  }

  void set(svgpp::tag::attribute::data_speed, double val) {
    qInfo() << "Data-Speed" << val;
  }

  void set(svgpp::tag::attribute::data_repeat, int val) {}
  void set(svgpp::tag::attribute::data_height, double val) {}
  void set(svgpp::tag::attribute::data_diode, int val) {}
  void set(svgpp::tag::attribute::data_zstep, double val) {}

  template <class IRI>
  void set(svgpp::tag::attribute::data_config_name, tag::iri_fragment,
           IRI const &fragment) {
    qInfo() << "xlink::href"
            << QString::fromStdString(
                   std::string(fragment.begin(), fragment.end()));
  }

  void set(svgpp::tag::attribute::data_config_name, RangedChar fragment) {
    qInfo() << "xlink::href"
            << QString::fromStdString(
                   std::string(fragment.begin(), fragment.end()));
  }

  using ObjectContext::set;
  using StylableContext::set;
};
