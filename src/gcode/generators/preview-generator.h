#include <sstream>
#include <gcode/generators/base-generator.h>

#ifndef PREVIEW_GENERATOR_H
#define PREVIEW_GENERATOR_H

/*
Generator for the preview
*/
class PreviewGenerator : public BaseGenerator {
public:
  class Path {
  public:
    Path(QPointF target, double speed, double power) : target_(target), speed_(speed), power_(power) {}

    double power_;
    double speed_;
    QPointF target_;
  };

  PreviewGenerator() : BaseGenerator() {
  }

  void moveTo(float x, float y, float speed) override {
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
    paths_ << Path(QPointF(x_, y_), speed_, power_);
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
    // TODO (Support actual G91)
    str_stream_ << "G91" << std::endl;
  }

  void home() override {
    x_ = 0;
    y_ = 0;
    paths_ << Path(QPointF(x_, y_), speed_, power_);
  }

  const QList<Path> &paths() const {
    return paths_;
  }

  QList<Path> paths_;
};

#endif