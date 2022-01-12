#include <QDebug>
#include <QColor>
#include <layer.h>
#include <parser/contexts/base-context.h>
#include <iostream>

#pragma once

namespace Parser {

class GroupContext : public BaseContext {
public:
  explicit GroupContext(BaseContext const &parent) :
       BaseContext(parent), is_sub_group_(false) {
    data_color_ = Qt::black;
    layer_ptr_ = nullptr;
    qInfo() << "<g>" << this;
  }

  explicit GroupContext(const GroupContext &parent) :
       BaseContext(parent), is_sub_group_(true) {
    data_color_ = Qt::black;
    layer_ptr_ = nullptr;
    qInfo() << "<gg>" << this;
  }

  void set(svgpp::tag::attribute::data_strength, double val) {
    qInfo() << "[SVGPP] Set layer data-Strength" << val;
    layer().setStrength(val);
  }

  void set(svgpp::tag::attribute::data_speed, double val) {
    qInfo() << "[SVGPP] Set layer data-Speed" << val;
    layer().setSpeed(val);
  }

  void set(svgpp::tag::attribute::data_repeat, int val) {
    layer().setRepeat(val);
  }

  void set(svgpp::tag::attribute::data_height, double val) {
    layer().setTargetHeight(val);
  }

  void set(svgpp::tag::attribute::data_diode, int val) {
    layer().setUseDiode(val);
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
    // TODO (load layer config)
    qInfo() << "[SVGPP] Layer config strong"
            << QString::fromStdString(
                 std::string(fragment.begin(), fragment.end()));
  }

  Layer &layer() {
    if (layer_ptr_ == nullptr) {
      qInfo() << "[SVGPP] Created an empty layer";
      layer_ptr_ = std::make_shared<Layer>();
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

      layer_ptr_ = nullptr;
      data_color_ = Qt::black;
    }
    if (is_sub_group_) {
      qInfo() << "</gg>";
    } else {
      qInfo() << "</g>";
    }
  }

  std::string type() {
    return "g";
  }

  using BaseContext::set;
  using ObjectContext::set;
  using StylableContext::set;
  QColor data_color_;
  LayerPtr layer_ptr_;
  bool is_sub_group_;
};

}
