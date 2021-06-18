#include <QDebug>
#include <QColor>
#include <layer.h>
#include <parser/contexts/base-context.h>
#include <iostream>

#pragma once

class GroupContext : public BaseContext {
public:
  GroupContext(BaseContext const &parent) : BaseContext(parent) {
    qInfo() << "<g>";
    layer_ptr_ = nullptr;
  }

  void set(svgpp::tag::attribute::data_strength, double val) {
    qInfo() << "Data-Strength" << val;
    layer().setStrength(val);
  }

  void set(svgpp::tag::attribute::data_speed, double val) {
    qInfo() << "Data-Speed" << val;
    layer().setSpeed(val);
  }

  void set(svgpp::tag::attribute::data_repeat, int val) {
    layer().setRepeat(val);
  }

  void set(svgpp::tag::attribute::data_height, double val) {
    layer().setTargetHeight(val);
  }

  void set(svgpp::tag::attribute::data_diode, int val) {
    layer().setDiode(val);
  }

  void set(svgpp::tag::attribute::data_zstep, double val) {
    layer().setStepHeight(val);
  }

  void set(svgpp::tag::attribute::data_name, RangedChar fragment) {
    layer().setName(QString::fromStdString(std::string(fragment.begin(), fragment.end())));
  }

  void set(svgpp::tag::attribute::data_color, RangedChar fragment) {
    data_color_ = QColor(QString::fromStdString(std::string(fragment.begin(), fragment.end())));
  }

  void set(svgpp::tag::attribute::data_config_name, RangedChar fragment) {
    qInfo() << "xlink::href"
            << QString::fromStdString(
                 std::string(fragment.begin(), fragment.end()));
  }

  Layer &layer() {
    if (layer_ptr_ == nullptr) {
      qInfo() << "Create layer";
      layer_ptr_ = make_shared<Layer>();
    }
    svgpp_set_active_layer(layer_ptr_);
    return *layer_ptr_;
  }

  void on_exit_element() {
    if (layer_ptr_ != nullptr) {
      layer_ptr_->setColor(data_color_);
      layer_ptr_->setType(Layer::Type::Mixed);
      svgpp_add_layer(layer_ptr_);
      svgpp_unset_active_layer();
    }
    qInfo() << "</g>";
  }

  string type() {
    return "g";
  }

  using BaseContext::set;
  using ObjectContext::set;
  using StylableContext::set;
  bool is_layer_;
  QColor data_color_;
  LayerPtr layer_ptr_;
};
