#include <QDebug>
#include <sstream>

#ifndef BASE_GENERATOR_H
#define BASE_GENERATOR_H

#define NO_BASIC_IMPL Q_ASSERT_X(true, "BaseGenerator", "Basic feature not implemented");
class BaseGenerator {
  public:
    BaseGenerator () : current_x_(0), current_y_(0), current_power_(0), current_speed_(0) {}
    virtual void moveTo(float x, float y, float speed) { NO_BASIC_IMPL }
    virtual void setLaserPowerLimit(float power) { NO_BASIC_IMPL }
    virtual void turnOffLaser() { NO_BASIC_IMPL }
    virtual void turnOnLaser() { NO_BASIC_IMPL }
    virtual void useAbsolutePositioning() {NO_BASIC_IMPL }
    virtual void useRelativePositioning() { NO_BASIC_IMPL }
    virtual void home() { NO_BASIC_IMPL }
    virtual void reset() { NO_BASIC_IMPL }
    // Advanced laser cutter features
    virtual void enableRotary() { Q_ASSERT_X(true, "BaseGenerator", "Rotary feature not implemented"); }
    virtual void disableRotary() { Q_ASSERT_X(true, "BaseGenerator", "Rotary feature not implemented"); }
    virtual void enableHighSpeedRastering() { Q_ASSERT_X(true, "BaseGenerator", "High-speed rastering not implemented"); }
    virtual void disableHighSpeedRastering() { Q_ASSERT_X(true, "BaseGenerator", "High-speed rastering not implemented"); }
    virtual void enableDiodeLaser() { Q_ASSERT_X(true, "BaseGenerator", "Diode laser not implemented"); }
    virtual void disableDiodeLaser() { Q_ASSERT_X(true, "BaseGenerator", "Diode laser not implemented"); }
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