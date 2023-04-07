#pragma once

#include <toolpath_exporter/generators/base-generator.h>

class DirtyAreaOutlineGenerator : public BaseGenerator {
public:
  DirtyAreaOutlineGenerator(const MachineSettings::MachineParam &machine, bool rotary_mode) : BaseGenerator() {
    rotary_mode_ = rotary_mode;
    if(rotary_mode_) {
      switch (machine.origin) {
        case MachineSettings::MachineParam::OriginType::RearRight:
        case MachineSettings::MachineParam::OriginType::FrontRight:
          machine_origin_ = MachineSettings::MachineParam::OriginType::RearRight;
          break;
        case MachineSettings::MachineParam::OriginType::RearLeft:
        case MachineSettings::MachineParam::OriginType::FrontLeft:
          machine_origin_ = MachineSettings::MachineParam::OriginType::RearLeft;
          break;
        default:
          break;
      }
    } else {
      machine_origin_ = machine.origin;
    }
    machine_height_ = machine.height;
    machine_width_ = machine.width;
  };

  /**
   * @brief Input position from canvas coordinate -> generate G-Code for machine coordinate
   * @param x "absolute" position in real world scale and canvas axis direction
   * @param y "absolute" position in real world scale and canvas axis direction
   * @param speed
   * @param power
   */
  void moveTo(float x, float y, float speed, float power, double x_backlash) override {
    if (power == 0) {
      return;
    }

    // 1. Handle the axis direction (convert from canvas to machine)
    switch (machine_origin_) {
      case MachineSettings::MachineParam::OriginType::RearRight:
        // Canvas x axis direction is opposite to machine coordinate
        x = machine_width_ - x;
        break;
      case MachineSettings::MachineParam::OriginType::FrontRight:
        // Canvas x, y axis directions are opposite to machine coordinate
        x = machine_width_ - x;
        y = machine_height_ - y;
        break;
      case MachineSettings::MachineParam::OriginType::RearLeft:
        // NORMAL canvas x, y axis directions are the same as machine coordinate
        break;
      case MachineSettings::MachineParam::OriginType::FrontLeft:
        // Canvas y axis direction is opposite to machine coordinate
        y = machine_height_ - y;
        break;
      default:
        break;
    }

    // 2 Limit x,y position inside the work area
    if (x > machine_width_) {
      x = machine_width_;
    } else if (x < 0) {
      x = 0;
    }
    if (!rotary_mode_ && y > machine_height_) {
      y = machine_height_;
    } else if (y < 0) {
      y = 0;
    }

    // 3. Update boundary
    if (x_min_ == -1 && x_max_ == -1) {
      x_min_ = x;
      x_max_ = x;
    } else if (x < x_min_) {
      x_min_ = x;
    } else if (x > x_max_) {
      x_max_ = x;
    }

    if (y_min_ == -1 && y_max_ == -1) {
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
    should_home_ = true;
  }

  void syncProgramFlow() override { 
  }

  void finishProgramFlow() override {
  }

  void reset() override {
    BaseGenerator::reset();
    machine_width_ = 0;
    machine_height_ = 0;
    machine_origin_ = MachineSettings::MachineParam::OriginType::RearLeft;
  }

  std::string toString() override {
    str_stream_.str(std::string()); // clear

    if (should_home_) {
      str_stream_ << "$H" << std::endl; // TODO: Ignore homing cmd? (otherwise, it's time consuming)
    }

    str_stream_ << "G90" << std::endl;
    str_stream_ << "G1F" << std::to_string(travel_speed_) << std::endl;
    str_stream_ << "G1S0" << std::endl;
    str_stream_ << "M3" << std::endl;
    if (x_min_ == x_max_ && x_min_ == -1) {
      str_stream_ << "G1S" << std::to_string(laser_power_ * 10) << std::endl;//from % to 1/1000
      str_stream_ << "G1S0" << std::endl;
    } else {
      str_stream_ << "G1" << "X" << round(x_min_ * 1000) / 1000 << "Y" << round(y_min_ * 1000) / 1000 << std::endl;
      str_stream_ << "G1S" << std::to_string(laser_power_ * 10) << std::endl;//from % to 1/1000
      str_stream_ << "G1" << "X" << round(x_max_ * 1000) / 1000 << "Y" << round(y_min_ * 1000) / 1000 << std::endl;
      str_stream_ << "G1" << "X" << round(x_max_ * 1000) / 1000 << "Y" << round(y_max_ * 1000) / 1000 << std::endl;
      str_stream_ << "G1" << "X" << round(x_min_ * 1000) / 1000 << "Y" << round(y_max_ * 1000) / 1000 << std::endl;
      str_stream_ << "G1" << "X" << round(x_min_ * 1000) / 1000 << "Y" << round(y_min_ * 1000) / 1000 << std::endl;
      str_stream_ << "G1S0" << std::endl;
    }
    str_stream_ << "M2" << std::endl; // // Sync program flow and End the program (clear state: turn off laser, turn off coolant, ...)
    return str_stream_.str();
  };

  void setTravelSpeed(double travel_speed) {travel_speed_ = travel_speed;}

  void setLaserPower(double laser_power) {laser_power_ = laser_power;}

private:
    int machine_width_;
    int machine_height_;
    qreal x_min_ = -1;
    qreal x_max_ = -1;
    qreal y_min_ = -1;
    qreal y_max_ = -1;
    MachineSettings::MachineParam::OriginType machine_origin_;
    double travel_speed_ = 6000;
    double laser_power_ = 2;
    bool should_home_ = false;
    bool rotary_mode_;
};
