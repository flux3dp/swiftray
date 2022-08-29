#pragma once

#include <toolpath_exporter/generators/base-generator.h>

class DirtyAreaOutlineGenerator : public BaseGenerator {
public:
  DirtyAreaOutlineGenerator(const MachineSettings::MachineSet &machine) : BaseGenerator() {
    machine_origin_ = machine.origin;
    machine_width_ = machine.width;
    machine_height_ = machine.height;
  };

  /**
   * @brief Input position from canvas coordinate -> generate G-Code for machine coordinate
   * @param x "absolute" position in real world scale and canvas axis direction
   * @param y "absolute" position in real world scale and canvas axis direction
   * @param speed
   * @param power
   */
  void moveTo(float x, float y, float speed, float power) override {
    if (power == 0) {
      return;
    }

    // 1. Handle the axis direction (convert from canvas to machine)
    switch (machine_origin_) {
      case MachineSettings::MachineSet::OriginType::RearRight:
        // Canvas x axis direction is opposite to machine coordinate
        x = machine_width_ - x;
        break;
      case MachineSettings::MachineSet::OriginType::FrontRight:
        // Canvas x, y axis directions are opposite to machine coordinate
        x = machine_width_ - x;
        y = machine_height_ - y;
        break;
      case MachineSettings::MachineSet::OriginType::RearLeft:
        // NORMAL canvas x, y axis directions are the same as machine coordinate
        break;
      case MachineSettings::MachineSet::OriginType::FrontLeft:
        // Canvas y axis direction is opposite to machine coordinate
        y = machine_height_ - y;
        break;
      default:
        break;
    }

    // 2. Update boundary
    if (x_min_ == 0 && x_max_ == 0) {
      x_min_ = x;
      x_max_ = x;
    } else if (x < x_min_) {
      x_min_ = x;
    } else if (x > x_max_) {
      x_max_ = x;
    }

    if (y_min_ == 0 && y_max_ == 0) {
      y_min_ = y;
      y_max_ = y;
    } else if (y < y_min_) {
      y_min_ = y;
    } else if (y > y_max_) {
      y_max_ = y;
    }

  }

  void setLaserPower(float power) override {
  }

  void turnOffLaser() override {
  }

  void turnOnLaser() override {
  }

  void turnOnLaserAdpatively() override {
  }

  void useAbsolutePositioning() override {
  }

  void useRelativePositioning() override {
  }

  void home() override {
  }

  void reset() override {
    BaseGenerator::reset();
    machine_width_ = 0;
    machine_height_ = 0;
    machine_origin_ = MachineSettings::MachineSet::OriginType::RearLeft;
  }

  std::string toString() override {
    str_stream_.str(std::string()); // clear

    str_stream_ << "$H" << std::endl; // TODO: Ignore homing cmd? (otherwise, it's time consuming)

    str_stream_ << "G90" << std::endl;
    str_stream_ << "G1F6000" << std::endl;
    str_stream_ << "G1S0" << std::endl;
    str_stream_ << "M3" << std::endl;
    str_stream_ << "G1" << "X" << round(x_min_ * 1000) / 1000 << "Y" << round(y_min_ * 1000) / 1000 << std::endl;
    str_stream_ << "G1S20" << std::endl;
    str_stream_ << "G1" << "X" << round(x_max_ * 1000) / 1000 << "Y" << round(y_min_ * 1000) / 1000 << std::endl;
    str_stream_ << "G1" << "X" << round(x_max_ * 1000) / 1000 << "Y" << round(y_max_ * 1000) / 1000 << std::endl;
    str_stream_ << "G1" << "X" << round(x_min_ * 1000) / 1000 << "Y" << round(y_max_ * 1000) / 1000 << std::endl;
    str_stream_ << "G1" << "X" << round(x_min_ * 1000) / 1000 << "Y" << round(y_min_ * 1000) / 1000 << std::endl;
    str_stream_ << "G1S0" << std::endl;
    str_stream_ << "M5" << std::endl;
    return str_stream_.str();
  };

private:
    int machine_width_;
    int machine_height_;
    qreal x_min_ = 0;
    qreal x_max_ = 0;
    qreal y_min_ = 0;
    qreal y_max_ = 0;
    MachineSettings::MachineSet::OriginType machine_origin_;
};
