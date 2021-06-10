#include <sstream>

#ifndef BASE_GENERATOR_H
#define BASE_GENERATOR_H

class BaseGenerator {
  public:
    BaseGenerator () : current_x_(0), current_y_(0), current_power_(0), current_speed_(0) {}
    virtual void moveTo(float x, float y, float speed) {}
    virtual void setLaserPowerLimit(float power) {}
    virtual void turnOffLaser() {}
    virtual void turnOnLaser() {}
    virtual void useAbsolutePositioning() {}
    virtual void useRelativePositioning() {}
    virtual void home() {}
    virtual void reset() {}
    std::string toString() const { return str_stream_.str(); };
    std::stringstream& stream() { return str_stream_; }

  protected:
    std::stringstream str_stream_;
    float current_x_;
    float current_y_;
    float current_power_;
    float current_speed_;
};

#endif