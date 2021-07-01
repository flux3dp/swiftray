#include <sstream>
#include <gcode/generators/base-generator.h>

#ifndef GCODE_GENERATOR_H
#define GCODE_GENERATOR_H

/*
Basic GCode Generator for Grbl like machines.
*/
class GCodeGenerator : public BaseGenerator {
public:
  GCodeGenerator() : BaseGenerator() {
  }

  void moveTo(float x, float y, float speed, float power) override {
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
};

#endif