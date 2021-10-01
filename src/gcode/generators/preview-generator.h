#pragma once

#include <sstream>
#include <gcode/generators/base-generator.h>

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

  void moveTo(float x, float y, float speed, float power) override {
    if (relative_mode_) {
      if (x != 0) {
        x_ = x_ + x;
      }
      if (y != 0) {
        y_ = y_ + y;
      }
      if (x == 0 && y == 0 && speed_ == speed && power_ == power) {
        return;
      }
    } else {
      if (x_ == x && y_ == y && speed_ == speed && power_ == power)
        return;
      if (x_ != x) {
        x_ = x;
      }
      if (y_ != y) {
        y_ = y;
      }
    }
    if (speed_ != speed) {
      speed_ = speed;
    }
    if (power_ != power) {
      power_ = power;
    }
    paths_ << Path(QPointF(x_, y_), speed_, power_);
  }

  void setLaserPower(float power) override {
    power_ = power;
  }

  void turnOffLaser() override {
    power_ = 0;
  }

  void turnOnLaser() override {
    power_ = 1;
  }

  void useAbsolutePositioning() override {
    relative_mode_ = false;
  }

  void useRelativePositioning() override {
    relative_mode_ = true;
  }

  void home() override {
    x_ = 0;
    y_ = 0;
    paths_ << Path(QPointF(x_, y_), speed_, power_);
  }

  const QList<Path> &paths() const {
    return paths_;
  }

  bool relative_mode_;
  QList<Path> paths_;
};
