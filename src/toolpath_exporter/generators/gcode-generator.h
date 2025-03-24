#pragma once

#include <sstream>
#include <toolpath_exporter/generators/base-generator.h>
#include <settings/machine-settings.h>
#include <cmath>
#include <QDebug>

/*
Basic GCode Generator for Grbl like machines.
*/
class GCodeGenerator : public BaseGenerator {
public:
  enum class GCodeMotionModal {
    kG00,
    kG01,
    kG02,
    kG03,
    // TBD: Add others only when needed
  };
  enum class GCodeDistanceModal {
    kG90, // absolute distance
    kG91  // relative distance
  };
  enum class MCodeSpindleModal {
    kM03, // constant power mode
    kM04, // adjust power based on actual_speed / nominal_speed
    kM05  // spindle off
  };

  GCodeGenerator(const MachineSettings::MachineParam &machine, bool rotary_mode) : BaseGenerator() {
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
    qInfo() << "Machine width: " << machine_width_ << " height: " << machine_height_;
  }

  /**
   * @brief Input position from canvas coordinate -> generate G-Code for machine coordinate
   * @param x "absolute" position in real world scale and canvas axis direction
   * @param y "absolute" position in real world scale and canvas axis direction
   * @param speed
   * @param power
   */
  void moveTo(float x, float y, float speed, float power, double x_backlash) override {
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

    // 1-2. Handle x direction backlash
    if (x > x_) {
      x += x_backlash;
    } else {
      x -= x_backlash;
    }

    // 2 Limit x,y position inside the work area
    //   NOTE: Also need to consider the precision error of floating point number
    //         so we add an epsilon here
    if (x > machine_width_ - epsilon_) {
      x = machine_width_ - epsilon_;
    } else if (x < epsilon_) {
      x = epsilon_;
    }
    if (!rotary_mode_ && y > machine_height_ - epsilon_) {
      y = machine_height_ - epsilon_;
    } else if (y < epsilon_) {
      y = epsilon_;
    }

    // 3. Separate relative mode & absolute mode
    if (std::fabs(x - x_) < epsilon_ && std::fabs(y - y_) < epsilon_ &&
        std::fabs(speed_ - speed) < epsilon_ &&
        std::fabs(power_ - power) < epsilon_)
      return;

    bool is_absolute = distance_modal_ == GCodeDistanceModal::kG90;
    int split = rotary_split_ > 0 ? y / rotary_split_ : 0;
    float split_start = 0;
    float split_end = rotary_split_;
    int split_dir = 1;
    if (split - split_ < 0) {
      split_start = rotary_split_;
      split_end = 0;
      split_dir = -1;
    }

    if (motion_modal_ != GCodeMotionModal::kG01) {
      str_stream_ << "G1";
      motion_modal_ = GCodeMotionModal::kG01;
    }
    if (std::fabs(speed_ - speed) >= epsilon_) {
      str_stream_ << "F" << speed * 60; // mm/s to mm/min
      speed_ = speed;
    }
    if (power > 0) {
      // Handle each split separately
      std::string resetPowerStr = "S";
      resetPowerStr += std::to_string(int(power * 10));
      resetPowerStr += "\n";
      while (split_ != split) {
        float next_y = split_ * rotary_split_ + split_end;
        float next_x = (x - x_) * (next_y - y_) / (y - y_) + x_;
        // Move to the end of the split and Turn on laser
        moveX(next_x, is_absolute);
        moveY(next_y, is_absolute);
        str_stream_ << resetPowerStr;
        // Move to next split (rotate) and Turn off laser
        rotate(split_ + split_dir);
        // Move to the start of the split
        moveY(next_y, is_absolute);
        str_stream_ << std::endl;
      }
    } else {
      // Directly move to the target split
      rotate(split);
    }
    moveX(x, is_absolute);
    moveY(y, is_absolute);
    if (std::fabs(power_ - power) >= epsilon_) {
      str_stream_ << "S" << power * 10;
      power_ = power;
    }
    str_stream_ << std::endl;
  }

  void moveZ(float z) override {
    // Always use relative mode for Z
    str_stream_ << "M102" << std::endl;
    str_stream_ << "Z" << std::round(z * move_precision_) / move_precision_ << std::endl;
  }

  void setLaserPower(float power) override {
    str_stream_ << "M3S" << power * 10 << std::endl;
    power_ = power;
  }

  void turnOffLaser() override {
    str_stream_ << "M5" << std::endl;
    spindle_modal_ = MCodeSpindleModal::kM05;
    power_ = 0;
  }

  void turnOnLaser() override {
    str_stream_ << "M3S0" << std::endl;
    spindle_modal_ = MCodeSpindleModal::kM03;
    power_ = 1;
  }

  void enableRotary() override {
    str_stream_ << "M101" << std::endl;
  }

  void disableRotary() override {
    str_stream_ << "M100" << std::endl;
  }

  void setRedLight(bool val) override {
    str_stream_ << (val ? "M103" : "M104") << std::endl;
  }

  void setWorkarea(QRectF workarea) override {
    str_stream_ << "W" << workarea.width() << std::endl;
  }

  void setFrequency(int frequency) override {
    str_stream_ << "Q" << frequency << std::endl;
  }

  void setPulseWidth(int pulse_width) override { 
    str_stream_ << "P" << pulse_width << std::endl;
  }

  void setDottingTime(int dotting_time) override { 
    str_stream_ << "T" << dotting_time << std::endl;
  }

  void setWobble(double wobble_step, double wobble_diameter) override {
    str_stream_ << "WS" << wobble_step << "WD" << wobble_diameter << std::endl;
    // Estimate wobble time multiplier (not accurate)
    double wobble_k = 1;
    if (wobble_step > 0 && wobble_diameter > 0) {
      wobble_k = M_PI * wobble_diameter / wobble_step + 1;
      if (wobble_step <= 0.1) {
        if (wobble_diameter <= 0.1) {
          wobble_k *= 2.5;
        } else if (wobble_diameter <= 0.2) {
          wobble_k *= wobble_step <= 0.01 ? 1.27 : 1.2;
        } else {
          wobble_k *= 1.05;
        }
      }
    }
    addComment(QString("WOBBLE K %1").arg(wobble_k));
  }

  void setRotary(double rotary_axis_coord,
                 double rotary_ratio,
                 double rotary_split,
                 double rotary_overlap) {
    rotary_axis_coord_ = rotary_axis_coord;
    rotary_ratio_ = rotary_ratio;
    rotary_split_ = rotary_split;
    rotary_offset_ = rotary_split_ / 2;
    rotary_overlap_ = rotary_overlap;
  }

  void addComment(QString msg) override {
    str_stream_ << ";" << msg.toStdString() << std::endl;
  }

  void turnOnLaserAdpatively() override {
    str_stream_ << "M4S0" << std::endl;
    spindle_modal_ = MCodeSpindleModal::kM04;
    power_ = 1;
  }

  void useAbsolutePositioning() override {
    str_stream_ << "G90" << std::endl;
    distance_modal_ = GCodeDistanceModal::kG90;
  }

  void useRelativePositioning() override {
    str_stream_ << "G91" << std::endl;
    distance_modal_ = GCodeDistanceModal::kG91;
  }

  void home() override {
    str_stream_ << "$H" << std::endl;
    x_ = y_ = 0;
  }

  void homeRotary(bool to_offset) override {
    if (to_offset) {
      // Force a move to axis center
      str_stream_ << "G1";
      split_ = -1;
      rotate(0);
      // Force y move to axis center
      y_ = -1;
      moveY(0, true);
      str_stream_ << std::endl;
    } else {
      str_stream_ << "A0S0" << std::endl;
    }
  }

  /**
   * @brief Wait until all motions in the buffer to finish
   * 
   */
  void syncProgramFlow() override { 
    str_stream_ << "M0" << std::endl;
  }

  /**
   * @brief Wait until all motions in the buffer to finish 
   *        and then clear state: turn off laser, turn off coolant, ...
   * 
   */
  void finishProgramFlow() override {
    str_stream_ << "M2" << std::endl;
  }

  void reset() override {
    BaseGenerator::reset();
    machine_width_ = 0;
    machine_height_ = 0;
    machine_origin_ = MachineSettings::MachineParam::OriginType::RearLeft;
    motion_modal_ = GCodeMotionModal::kG00;
    distance_modal_ = GCodeDistanceModal::kG90;
    spindle_modal_ = MCodeSpindleModal::kM05;
  }

private:
  int machine_width_;
  int machine_height_;
  GCodeMotionModal motion_modal_ = GCodeMotionModal::kG00;
  GCodeDistanceModal distance_modal_ = GCodeDistanceModal::kG90;
  MCodeSpindleModal spindle_modal_ = MCodeSpindleModal::kM05;
  MachineSettings::MachineParam::OriginType machine_origin_;
  float epsilon_ = 0.00005;
  float move_precision_ = 10000; // 10000 for Promark, 1000 for other machines
  double rotary_ratio_ = 1;
  double rotary_axis_coord_ = 0;  // rotary center (blue line), mm
  double rotary_split_ = 0;    // height of each split, 0 means no splitting, mm
  double rotary_offset_ = 0;   // center of split, mm
  double rotary_overlap_ = 0;  // mm
  double y_in_split_ = 0;      // relative y_ to the current split center
  int split_ = 0;

  void moveX(float target_x, bool is_absolute) {
    if (std::fabs(target_x - x_) < epsilon_) {
      return;
    }
    float arg_x = is_absolute ? target_x : target_x - x_;
    arg_x = std::round(arg_x * move_precision_) / move_precision_;
    str_stream_ << "X" << arg_x;
    x_ = is_absolute ? target_x : x_ + arg_x;
  }

  void moveY(float target_y, bool is_absolute) {
    if (std::fabs(target_y - y_) < epsilon_) {
      return;
    }
    float split_base = split_ * (rotary_split_ - rotary_overlap_) + rotary_offset_;
    float target_y_in_split_ = target_y - split_base;
    float arg_y = is_absolute ? (target_y_in_split_ + rotary_axis_coord_)
                              : (target_y_in_split_ - y_in_split_);
    arg_y = std::round(arg_y * move_precision_) / move_precision_;
    str_stream_ << "Y" << arg_y;
    y_ = is_absolute ? target_y : y_ + arg_y;
    y_in_split_ = target_y_in_split_;
  }

  void rotate(int target_split) {
    // Always use absolute mode for rotary
    if (target_split == split_) {
      return;
    }
    float arg_a = target_split * (rotary_split_ - rotary_overlap_) + rotary_offset_;
    arg_a = std::round(arg_a * rotary_ratio_ * move_precision_) / move_precision_;
    str_stream_ << "A" << arg_a << "S0" << std::endl;
    power_ = 0;
    split_ = target_split;
    y_ = split_ * rotary_split_ + rotary_offset_ + y_in_split_;
  }
};
