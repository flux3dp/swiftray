#include <QDebug>
#include <sstream>

#ifndef BASE_GENERATOR_H
#define BASE_GENERATOR_H

#define NO_BASIC_IMPL Q_ASSERT_X(false, "BaseGenerator", "Basic feature not implemented");
class BaseGenerator {
  public:
    BaseGenerator () : x_(0), y_(0), power_(0), speed_(0) {}
    virtual void moveTo(float x, float y, float speed) { NO_BASIC_IMPL }
    virtual void moveToX(float x) { moveTo(x, y_, speed_); }
    virtual void setSpeed(float speed) { moveTo(x_, y_, speed_); }
    virtual void setLaserPower(float power) { NO_BASIC_IMPL }
    virtual void turnOffLaser() { NO_BASIC_IMPL }
    virtual void turnOnLaser() { NO_BASIC_IMPL }
    virtual void useAbsolutePositioning() {NO_BASIC_IMPL }
    virtual void useRelativePositioning() { NO_BASIC_IMPL }
    virtual void home() { NO_BASIC_IMPL }
    virtual void reset() {
        x_ = 0;
        y_ = 0;
        power_ = 0;
        speed_ = 0;
    }
    // Advanced laser cutter features
    virtual void enableRotary() { Q_ASSERT_X(true, "BaseGenerator", "Rotary feature not implemented"); }
    virtual void disableRotary() { Q_ASSERT_X(true, "BaseGenerator", "Rotary feature not implemented"); }
    virtual void enableHighSpeedRastering() { Q_ASSERT_X(true, "BaseGenerator", "High-speed rastering not implemented"); }
    virtual void disableHighSpeedRastering() { Q_ASSERT_X(true, "BaseGenerator", "High-speed rastering not implemented"); }
    virtual void enableDiodeLaser() { Q_ASSERT_X(true, "BaseGenerator", "Diode laser not implemented"); }
    virtual void disableDiodeLaser() { Q_ASSERT_X(true, "BaseGenerator", "Diode laser not implemented"); }
    virtual void beginHighSpeedRastering(int pixels) { Q_ASSERT_X(true, "BaseGenerator", "High-speed rastering not implemented"); }
    virtual void pushRasteringPixels32(int32_t bit) { Q_ASSERT_X(true, "BaseGenerator", "High-speed rastering not implemented"); }
    virtual void endHighSpeedRastering() { Q_ASSERT_X(true, "BaseGenerator", "High-speed rastering not implemented"); }
    std::string toString() const { return str_stream_.str(); };
    std::stringstream& stream() { return str_stream_; }

    float power() { return power_; }
    float x() { return x_; }
    float y() { return y_; }
    float speed() { return speed_; }

  protected:
    std::stringstream str_stream_;
    float x_;
    float y_;
    float power_;
    float speed_;
};

#endif