#include <sstream>
#include <gcode/generators/base_generator.h>

#ifndef GCODE_GENERATOR_H
#define GCODE_GENERATOR_H

/*
Basic GCode Generator for Grbl like machines.
*/
class GCodeGenerator : public BaseGenerator {
  public:
    GCodeGenerator() : BaseGenerator() {
    }

    void moveTo(float x, float y, float speed) override{
        if (x_ == x && y_ == y && speed_ == speed)
            return;
        str_stream_ << "G1";
        if (x_ != x) {
            str_stream_ << "X" << x;
            x_ = x;
        }
        if (y_ != y) {
            str_stream_ << "Y" << y;
            y_ = y;
        }
        if (speed_ != speed) {
            str_stream_ << "F" << speed;
            speed_ = speed;
        }
        str_stream_ << std::endl;
    }

    void setLaserPower(float power) override {
        str_stream_ << "M3S" << power << std::endl;
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
        str_stream_ << "G91" << std::endl;
    }

    void home() override {
        str_stream_ << "G28" << std::endl;
    }
};

#endif