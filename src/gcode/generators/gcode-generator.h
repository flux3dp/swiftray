#pragma once

#include <sstream>
#include <gcode/generators/base-generator.h>
#include <settings/machine-settings.h>

/*
Basic GCode Generator for Grbl like machines.
*/
class GCodeGenerator : public BaseGenerator {
public:
  GCodeGenerator(const MachineSettings::MachineSet &machine) : BaseGenerator() {
    machine_width_ = machine.width;
    machine_height_ = machine.height;
    machine_origin_ = machine.origin;
  }

  void moveTo(float x, float y, float speed, float power) override {
    // Coordinate transform for different origin type
    switch (machine_origin_) {
      case MachineSettings::MachineSet::OriginType::RearLeft:
        break;
      case MachineSettings::MachineSet::OriginType::RearRight:
        x = machine_width_ - x;
        break;
      case MachineSettings::MachineSet::OriginType::FrontLeft:
        y = machine_height_ - y;
        break;
      case MachineSettings::MachineSet::OriginType::FrontRight:
        y = machine_height_ - y;
        x = machine_width_ - x;
        break;
    }
    if (x_ == x && y_ == y && speed_ == speed && power_ == power)
      return;
    str_stream_ << "G1";
    if (x_ != x) {
      str_stream_ << "X" << round(x * 1000) / 1000;
      x_ = x;
    }
    if (y_ != y) {
      str_stream_ << "Y" << round(y * 1000) / 1000;
      y_ = y;
    }
    if (speed_ != speed) {
      str_stream_ << "F" << speed * 60; // mm/s to mm/min
      speed_ = speed;
    }

    if (power_ != power) {
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
    power_ = 0;
  }

  void turnOnLaser() override {
    str_stream_ << "M3" << std::endl;
    power_ = 1;
  }

  void useAbsolutePositioning() override {
    str_stream_ << "G90" << std::endl;
  }

  void useRelativePositioning() override {
    // TODO (Support actual G91)
    str_stream_ << "G91" << std::endl;
  }

  void home() override {
    str_stream_ << "$H" << std::endl;
    x_ = y_ = 0;
  }

private:
  int machine_width_;
  int machine_height_;
  MachineSettings::MachineSet::OriginType machine_origin_;
};