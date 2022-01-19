#pragma once

#include <sstream>
#include <gcode/generators/base-generator.h>
#include <settings/machine-settings.h>

/**
 * @brief 
 *        Generator for the preview
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

  PreviewGenerator(const MachineSettings::MachineSet &machine) : BaseGenerator() {
    machine_origin_ = machine.origin;
    machine_width_ = machine.width;
    machine_height_ = machine.height;

    // Initialize to home position (NOTE: Machines' home are NOT necessarily (0,0))
    // TBD: Home command appended by toolpath-exporter or by generator itself?
    //home();
  }

  /**
   * @brief Input position from canvas coordinate -> generate G-Code for machine coordinate
   * @param x "absolute" position in real world scale and canvas axis direction
   * @param y "absolute" position in real world scale and canvas axis direction
   * @param speed
   * @param power
   */
  void moveTo(float x, float y, float speed, float power) override {
    if (x_ == x && y_ == y && speed_ == speed && power_ == power)
      return;

    if (x_ != x) {
      x_ = x;
    }
    if (y_ != y) {
      y_ = y;
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
    power_ = 0; // typically for determining preview track color
  }

  void turnOnLaser() override {
    power_ = 1; // typically for determining preview track color
  }

  void useAbsolutePositioning() override {
    relative_mode_ = false;
  }

  void useRelativePositioning() override {
    relative_mode_ = true;
  }

  void home() override {
    // NOTE: Not guaranteed ->
    //       We presume homing position is always at the "top left" corner here
    switch (machine_origin_) {
      case MachineSettings::MachineSet::OriginType::RearRight:
        // Canvas x axis direction is opposite to machine coordinate
        x_ = machine_width_;
        y_ = 0;
        break;
      case MachineSettings::MachineSet::OriginType::FrontRight:
        // Canvas x, y axis directions are opposite to machine coordinate
        x_ = machine_width_;
        y_ = machine_height_;
        break;
      case MachineSettings::MachineSet::OriginType::RearLeft:
        // NORMAL canvas x, y axis directions are the same as machine coordinate
        x_ = y_ = 0;
        break;
      case MachineSettings::MachineSet::OriginType::FrontLeft:
        // Canvas y axis direction is opposite to machine coordinate
        x_ = 0;
        y_ = machine_height_;
        break;
      default:
        break;
    }

    paths_ << Path(QPointF(x_, y_), kHomingSpeed, 0);
  }

  const QList<Path> &paths() const {
    return paths_;
  }

  void reset() override {
    BaseGenerator::reset();
    relative_mode_ = false;
    machine_width_ = 0;
    machine_height_ = 0;
    machine_origin_ = MachineSettings::MachineSet::OriginType::RearLeft;
    paths_.clear();
  }

  bool relative_mode_;
  int machine_width_;
  int machine_height_;
  MachineSettings::MachineSet::OriginType machine_origin_;
  const float kHomingSpeed = 10000;

  QList<Path> paths_;
};
