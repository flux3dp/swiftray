#pragma once

#include <QDebug>
#include <sstream>

#define NO_BASIC_IMPL Q_ASSERT_X(false, "BaseGenerator", "Basic feature not implemented");

class BaseGenerator {
public:
  BaseGenerator() : x_(0), y_(0), power_(0), speed_(0) {}

  // x,y: absolute position in unit of mm
  virtual void moveTo(float x, float y, float speed, float power) { NO_BASIC_IMPL }

  virtual void moveToX(float x) { moveTo(x, y_, speed_, power_); }

  virtual void setSpeed(float speed) { moveTo(x_, y_, speed_, power_); }

  virtual void setLaserPower(float power) { NO_BASIC_IMPL }

  virtual void turnOffLaser() { NO_BASIC_IMPL }

  virtual void turnOnLaser() { NO_BASIC_IMPL }

  virtual void turnOnLaserAdpatively() { NO_BASIC_IMPL }

  virtual void useAbsolutePositioning() { NO_BASIC_IMPL }

  virtual void useRelativePositioning() { NO_BASIC_IMPL }

  virtual void home() { NO_BASIC_IMPL }

  virtual void reset() {
    x_ = 0;
    y_ = 0;
    power_ = 0;
    speed_ = 0;
  }

  virtual void appendCustomCmd(const std::string& cmd) { str_stream_ << cmd; }
  virtual void appendCustomCmd(std::string&& cmd) { str_stream_ << cmd; }

  // Advanced laser cutter features
  virtual void enableRotary() { Q_ASSERT_X(true, "BaseGenerator", "Rotary feature not implemented"); }

  virtual void disableRotary() { Q_ASSERT_X(true, "BaseGenerator", "Rotary feature not implemented"); }

  virtual void enableHighSpeedRastering() { Q_ASSERT_X(true, "BaseGenerator", "High-speed rastering not implemented"); }

  virtual void disableHighSpeedRastering() {
    Q_ASSERT_X(true, "BaseGenerator", "High-speed rastering not implemented");
  }

  virtual void enableDiodeLaser() { Q_ASSERT_X(true, "BaseGenerator", "Diode laser not implemented"); }

  virtual void disableDiodeLaser() { Q_ASSERT_X(true, "BaseGenerator", "Diode laser not implemented"); }

  virtual void beginHighSpeedRastering(int pixels) {
    Q_ASSERT_X(true, "BaseGenerator", "High-speed rastering not implemented");
  }

  virtual void pushRasteringPixels32(int32_t bit) {
    Q_ASSERT_X(true, "BaseGenerator", "High-speed rastering not implemented");
  }

  virtual void endHighSpeedRastering() { Q_ASSERT_X(true, "BaseGenerator", "High-speed rastering not implemented"); }

  virtual std::string toString() { return str_stream_.str(); };

  std::stringstream &stream() { return str_stream_; }

  float power() { return power_; }

  float x() { return x_; }

  float y() { return y_; }

  float speed() { return speed_; }

protected:
  std::stringstream str_stream_;
  float x_; // in canvas coordinate -> have nothing to do with machine coordinate
  float y_; // in canvas coordinate -> have nothing to do with machine coordinate
  float power_;
  float speed_;
};
