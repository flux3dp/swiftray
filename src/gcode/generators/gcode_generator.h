#include <sstream>
#include <gcode/generators/base_generator.h>

#ifndef GCODE_GENERATOR_H
#define GCODE_GENERATOR_H

class GCodeGenerator : public BaseGenerator {
  public:
    GCodeGenerator() : BaseGenerator() {
        reset();
    }

    void moveTo(float x, float y, float speed) override{
        if (current_x_ == x && current_y_ == y)
            return;
        str_stream_ << "G1";
        if (current_x_ != x)
            str_stream_ << "X" << x;
        if (current_y_ != y)
            str_stream_ << "Y" << y;
        if (current_speed_ != speed)
            str_stream_ << "F" << speed;
        str_stream_ << std::endl;
    }

    void setLaserPowerLimit(float power) override {
        str_stream_ << "M3S" << power << std::endl;
    }
    
    void turnOffLaser() override {
        str_stream_ << "M5" << std::endl;
    }

    void turnOnLaser() override {
        str_stream_ << "M3" << std::endl;
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

    void reset() override {
        current_x_ = 0;
        current_y_ = 0;
        current_power_ = 0;
        current_speed_ = 0;
    }
};

#endif