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
    float x = 0;
    float y = 0;
    apply_axis_direction(x, y);
    cur_x_ = x;
    cur_y_ = y;
  };

  /**
   * @brief Input position from canvas coordinate -> generate G-Code for machine coordinate
   * @param x "absolute" position in real world scale and canvas axis direction
   * @param y "absolute" position in real world scale and canvas axis direction
   * @param speed
   * @param power
   */
  void moveTo(float x, float y, float speed, float power, double x_backlash) override {

    // 1. Handle the axis direction (convert from canvas to machine)
    apply_axis_direction(x, y);

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
    float origin_x = cur_x_;
    float origin_y = cur_y_;
    cur_x_ = x;
    cur_y_ = y;

    if (power == 0) {
      return;
    }
    update_boundary(origin_x, origin_y);
    update_boundary(x, y);
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

  void setWorkarea(QRectF workarea) override {
    machine_width_ = workarea.width();
    machine_height_ = workarea.height();
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
    str_stream_ << "W" << machine_width_ << std::endl;
    if (x_min_ == x_max_ && x_min_ == -1) {
      str_stream_ << "G1S" << std::to_string(laser_power_ * 10) << std::endl;//from % to 1/1000
      str_stream_ << "G1S0" << std::endl;
    } else if (rotary_mode_ && rotary_split_ > 0) {
      handleSplitedRotary();
    } else {
      handleBox(x_min_, x_max_, y_min_, y_max_);
    }
    str_stream_ << "M2" << std::endl; // // Sync program flow and End the program (clear state: turn off laser, turn off coolant, ...)
    return str_stream_.str();
  };

  void setTravelSpeed(double travel_speed) {travel_speed_ = travel_speed;}

  void setLaserPower(double laser_power) {laser_power_ = laser_power;}

  void setStep(double step) {
    if (step > 0) step_ = step;
  }

  void setRotary(double rotary_axis_coord,
                 double rotary_y_ratio,
                 double rotary_split,
                 double rotary_overlap) {
    rotary_axis_coord_ = rotary_axis_coord;
    rotary_y_ratio_ = rotary_y_ratio;
    rotary_split_ = rotary_split;
    rotary_offset_ = rotary_split_ / 2;
    rotary_overlap_ = rotary_overlap;
  }

  void update_boundary(float x, float y) {
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

private:
  int machine_width_;
  int machine_height_;
  qreal cur_x_ = 0;
  qreal cur_y_ = 0;
  qreal step_ = 0;
  qreal x_min_ = -1;
  qreal x_max_ = -1;
  qreal y_min_ = -1;
  qreal y_max_ = -1;
  MachineSettings::MachineParam::OriginType machine_origin_;
  double travel_speed_ = 6000;
  double laser_power_ = 2;
  bool should_home_ = false;
  bool rotary_mode_;
  double rotary_y_ratio_ = 1;
  double rotary_axis_coord_ = 0;
  double rotary_split_ = 0;
  double rotary_offset_ = 0;
  double rotary_overlap_ = 0;
  int split_ = 0;

  void apply_axis_direction(float &x, float &y) {
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
  }

  double getX(float x) { return std::round(x * 1000) / 1000; }

  double getY(float y) {
    float split_base = split_ * rotary_split_ + rotary_offset_;
    float arg_y = y - split_base + rotary_axis_coord_;
    return std::round(arg_y * 1000) / 1000;
  }

  double getA(int split) {
    split_ = split;
    float split_base = split_ * (rotary_split_ - rotary_overlap_) + rotary_offset_;
    return std::round(split_base * rotary_y_ratio_ * 1000) / 1000;
  }

  void handleBox(double x_min, double x_max, double y_min, double y_max) {
    str_stream_ << "G1" << "X" << getX(x_min) << "Y" << getY(y_min) << std::endl;
    str_stream_ << "G1S" << std::to_string(laser_power_ * 10) << std::endl; // from % to 1/1000
    if (step_ > 0) {
      for (int i = 1; x_min + i * step_ < x_max; i++) {
        str_stream_ << "G1" << "X" << getX(x_min + i * step_) << std::endl;
      }
    }
    str_stream_ << "G1" << "X" << getX(x_max) << "Y" << getY(y_min) << std::endl;
    if (step_ > 0) {
      for (int i = 1; y_min + i * step_ < y_max; i++) {
        str_stream_ << "G1" << "Y" << getY(y_min + i * step_) << std::endl;
      }
    }
    str_stream_ << "G1" << "X" << getX(x_max) << "Y" << getY(y_max) << std::endl;
    if (step_ > 0) {
      for (int i = 1; x_max - i * step_ > x_min; i++) {
        str_stream_ << "G1" << "X" << getX(x_max - i * step_) << std::endl;
      }
    }
    str_stream_ << "G1" << "X" << getX(x_min) << "Y" << getY(y_max) << std::endl;
    if (step_ > 0) {
      for (int i = 1; y_max - i * step_ > y_min; i++) {
        str_stream_ << "G1" << "Y" << getY(y_max - i * step_) << std::endl;
      }
    }
    str_stream_ << "G1" << "X" << getX(x_min) << "Y" << getY(y_min) << std::endl;
    str_stream_ << "G1S0" << std::endl;
  }

  void handleSplitedRotary() {
    split_ = 0;
    str_stream_ << "G1S0A" << getA(0) << std::endl;
    for (int current_split = y_min_ / rotary_split_; current_split <= y_max_ / rotary_split_; current_split++) {
      double y_min = std::max(current_split * rotary_split_, y_min_);
      double y_max = std::min((current_split + 1) * rotary_split_, y_max_);
      str_stream_ << "G1A" << getA(current_split) << std::endl;
      handleBox(x_min_, x_max_, y_min, y_max);
      handleBox(x_min_, x_max_, y_min, y_max);
      handleBox(x_min_, x_max_, y_min, y_max);
    }
    str_stream_ << "G1A0" << std::endl;
  }
};
