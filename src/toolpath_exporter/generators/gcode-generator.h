#pragma once

#include <sstream>
#include <toolpath_exporter/generators/base-generator.h>
#include <settings/machine-settings.h>
#include <cmath>

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

  GCodeGenerator(const MachineSettings::MachineSet &machine) : BaseGenerator() {
    machine_width_ = machine.width;
    machine_height_ = machine.height;
    machine_origin_ = machine.origin;
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
    if (y > machine_height_ - epsilon_) {
      y = machine_height_ - epsilon_;
    } else if (y < epsilon_) {
      y = epsilon_;
    }

    // 3. Separate relative mode & absolute mode
    if (distance_modal_ == GCodeDistanceModal::kG91) { // G91: relative distance
      if (std::fabs(x - x_) >= epsilon_ || std::fabs(y - y_) >= epsilon_) {
        if ( motion_modal_ != GCodeMotionModal::kG01) {
          str_stream_ << "G1";
          motion_modal_ = GCodeMotionModal::kG01;
        }
      }
      if (std::fabs(x - x_) >= epsilon_) {
        float dist_x = round((x - x_) * 1000) / 1000;
        str_stream_ << "X" << dist_x;
        x_ = x_ + dist_x;
      }
      if (std::fabs(y - y_) >= epsilon_) {
        float dist_y = std::round((y - y_) * 1000) / 1000;
        str_stream_ << "Y" << dist_y;
        y_ = y_ + dist_y;
      }
    } else { // G90: absolute distance
      // Coordinate transform for different origin type
      if (std::fabs(x - x_) < epsilon_ && std::fabs(y - y_) < epsilon_ 
          && std::fabs(speed_ - speed) < epsilon_ && std::fabs(power_ - power) < epsilon_)
        return;

      if ( motion_modal_ != GCodeMotionModal::kG01) {
        str_stream_ << "G1";
        motion_modal_ = GCodeMotionModal::kG01;
      }
      if (std::fabs(x - x_) >= epsilon_) {
        str_stream_ << "X" << std::round(x * 1000) / 1000;
        x_ = x;
      }
      if (std::fabs(y - y_) >= epsilon_) {
        str_stream_ << "Y" << std::round(y * 1000) / 1000;
        y_ = y;
      }
    }

    if (std::fabs(speed_ - speed) >= epsilon_) {
      str_stream_ << "F" << speed * 60; // mm/s to mm/min
      speed_ = speed;
    }

    if (std::fabs(power_ - power) >= epsilon_) {
      str_stream_ << "S" << power * 10; // mm/s to mm/min
      power_ = power;
    }
    str_stream_ << std::endl;
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
    machine_origin_ = MachineSettings::MachineSet::OriginType::RearLeft;
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
  MachineSettings::MachineSet::OriginType machine_origin_;
  float epsilon_ = 0.001;
};
