#pragma once

#include <sstream>
#include <toolpath_exporter/generators/base-generator.h>
#include <settings/machine-settings.h>
#include <QVector2D>
#include <QtMath>
#include <QMutex>

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

    if (speed_ != speed) {
      speed_ = speed;
    }
    if (power_ != power) {
      power_ = power;
    }

    // Insert sub path
    if (high_speed_raster_mode_) {
      QVector2D vect{x - x_, y-y_};
      vect.normalize();
      while (true) {
        if (high_speed_raster_idx_ >= high_speed_raster_pc_ ||
            (high_speed_raster_idx_ / 32) > (high_speed_raster_array_.size() - 1)) {
          if (high_speed_raster_emitting) {
            paths_mutex_.lock();
            paths_ << Path(QPointF(x_, y_), speed_, power_);
            paths_mutex_.unlock();
            high_speed_raster_emitting = false;
          }
          break;
        }
        if (high_speed_raster_remaining_mm_to_next_dot_ <=
            qSqrt(qPow(x - x_, 2) + qPow(y - y_, 2))) {
          x_ += (vect.x() * high_speed_raster_remaining_mm_to_next_dot_);
          y_ += (vect.y() * high_speed_raster_remaining_mm_to_next_dot_);
          bool next_emit_state = (high_speed_raster_array_[high_speed_raster_idx_ / 32]) & ((uint32_t)(0x80000000) >> (high_speed_raster_idx_ % 32));
          if (next_emit_state != high_speed_raster_emitting) {
            paths_mutex_.lock();
            paths_ << Path(QPointF(x_, y_), speed_, high_speed_raster_emitting ? power_ : 0);
            paths_mutex_.unlock();
            high_speed_raster_emitting  = next_emit_state;
          }
          high_speed_raster_remaining_mm_to_next_dot_ = mm_per_dot_;
          high_speed_raster_idx_ += 1;
        } else {
          high_speed_raster_remaining_mm_to_next_dot_ -= qSqrt(qPow(x - x_, 2) + qPow(y - y_, 2));
          break;
        }
      }
    }

    if (x_ != x) {
      x_ = x;
    }
    if (y_ != y) {
      y_ = y;
    }

    paths_mutex_.lock();
    paths_ << Path(QPointF(x_, y_), speed_,
                   high_speed_raster_mode_ ? high_speed_raster_emitting ? power_ : 0
                                                  : power_);
    paths_mutex_.unlock();
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

  void turnOnLaserAdpatively() override {
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

    paths_mutex_.lock();
    paths_ << Path(QPointF(x_, y_), kHomingSpeed, 0);
    paths_mutex_.unlock();
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
    paths_mutex_.lock();
    paths_.clear();
    paths_mutex_.unlock();
  }

  void appendCustomCmd (const std::string& cmd) override {
    // parse D0, D1, D2, D3, D4, D5 cmd
    QString q_str{cmd.c_str()};
    if (q_str.startsWith("D0R") &&q_str.size() >= 4) {
      if (cmd.at(3) == 'U') {
        mm_per_dot_ = 0.025;
      } else if (cmd.at(3) == 'H') {
        mm_per_dot_ = 0.05;
      } else if (cmd.at(3) == 'M') {
        mm_per_dot_ = 0.1;
      } else if (cmd.at(3) == 'L') {
        mm_per_dot_ = 0.2;
      }
      high_speed_raster_mode_ = true;
    } else if (q_str.startsWith("D1PC")) {
      QRegularExpression re("D1PC([0-9]+)");
      QRegularExpressionMatch match = re.match(q_str);
      if (match.hasMatch()) {
        high_speed_raster_pc_ = match.captured(1).toUInt();
      }
      high_speed_raster_array_.clear();
      high_speed_raster_idx_ = 0;
      high_speed_raster_remaining_mm_to_next_dot_ = 0;
      high_speed_raster_emitting = false;
    } else if (q_str.startsWith("D2W")) {
      auto i = 3;
      while (cmd.size() - i >= 8) {
        high_speed_raster_array_.push_back(std::stoul(cmd.substr(i, 8), nullptr, 16));
        i += 8;
      }
    } else if (q_str.startsWith("D3FE") || q_str.startsWith("D4PL")) {
      // do nothing
    } else if (q_str.startsWith("D5")) {
      high_speed_raster_mode_ = false;
    }

  }
  void appendCustomCmd (std::string&& cmd) override {
    // parse D0, D1, D2, D3, D4, D5 cmd
    QString q_str{cmd.c_str()};
    if (q_str.startsWith("D0R") &&q_str.size() >= 4) {
      if (cmd.at(3) == 'U') {
        mm_per_dot_ = 0.025;
      } else if (cmd.at(3) == 'H') {
        mm_per_dot_ = 0.05;
      } else if (cmd.at(3) == 'M') {
        mm_per_dot_ = 0.1;
      } else if (cmd.at(3) == 'L') {
        mm_per_dot_ = 0.2;
      }
      high_speed_raster_mode_ = true;
    } else if (q_str.startsWith("D1PC")) {
      QRegularExpression re("D1PC([0-9]+)");
      QRegularExpressionMatch match = re.match(q_str);
      if (match.hasMatch()) {
        high_speed_raster_pc_ = match.captured(1).toUInt();
      }
      high_speed_raster_array_.clear();
      high_speed_raster_idx_ = 0;
      high_speed_raster_remaining_mm_to_next_dot_ = 0;
      high_speed_raster_emitting = false;
    } else if (q_str.startsWith("D2W")) {
      auto i = 3;
      while (cmd.size() - i >= 8) {
        high_speed_raster_array_.push_back(std::stoul(cmd.substr(i, 8), nullptr, 16));
        i += 8;
      }
    } else if (q_str.startsWith("D3FE") || q_str.startsWith("D4PL")) {
      // do nothing
    } else if (q_str.startsWith("D5")) {
      high_speed_raster_mode_ = false;
    }
  }


  bool relative_mode_ = false;
  bool high_speed_raster_mode_ = false;
  uint32_t high_speed_raster_pc_ = 0;
  qreal high_speed_raster_remaining_mm_to_next_dot_ = 0;
  bool high_speed_raster_emitting = false;
  uint32_t high_speed_raster_idx_ = 0;
  QList<uint32_t> high_speed_raster_array_;
  int machine_width_;
  int machine_height_;
  MachineSettings::MachineSet::OriginType machine_origin_;
  const qreal kHomingSpeed = 10000;
  qreal mm_per_dot_ = 0.1;

  QList<Path> paths_;
  QMutex paths_mutex_;
};
