#include "rotary_test_job.h"

#include "../operation_cmd/gcode_cmd.h"
#include <string>

RotaryTestJob::RotaryTestJob(QRectF bbox, 
                            char rotary_axis, 
                            qreal feedrate, 
                            double framing_power, 
                            QString job_name) 
  : MachineJob{job_name}
{
  gcode_list_.push_back("G91");
  gcode_list_.push_back("M3");
  gcode_list_.push_back(QString::fromStdString("G1S" + std::to_string(framing_power * 10)));//from % to 1/1000
  gcode_list_.push_back(QString::fromStdString("G1 X" + std::to_string(bbox.width()) + "F" + std::to_string(feedrate)));
  gcode_list_.push_back(QString::fromStdString("G1 " + std::string(1, rotary_axis) + std::to_string(bbox.height())));
  gcode_list_.push_back(QString::fromStdString("G1 X" + std::to_string(-1 * bbox.width())));
  gcode_list_.push_back(QString::fromStdString("G1 " + std::string(1, rotary_axis) + std::to_string(-1 * bbox.height())));
  gcode_list_.push_back("M5");
  gcode_list_.push_back("G90");
  gcode_list_.push_back("M2");
  gcode_list_.push_back("?");  // Get the realtime position right at the end of job
}

void RotaryTestJob::setMotionController(QPointer<MotionController> motion_controller) {
  motion_controller_ = motion_controller;
}

std::shared_ptr<OperationCmd> RotaryTestJob::getNextCmd() {
  if (end()) {
    return std::shared_ptr<OperationCmd>{nullptr};
  }
  auto idx = next_gcode_idx_++;
  auto cmd = std::make_shared<GCodeCmd>();
  cmd->setGCode(gcode_list_.at(idx) + "\n");
  //cmd->setTarget(OperationCmd::Target::kMotionControl);
  cmd->setMotionController(motion_controller_);
  return cmd;
}

bool RotaryTestJob::end() const {
  if (next_gcode_idx_ >= gcode_list_.size()) {
    return true;
  }
  return false;
}

void RotaryTestJob::reload() {
  next_gcode_idx_ = 0;
}

float RotaryTestJob::getProgressPercent() const {
  if (gcode_list_.isEmpty()) {
    return 100;
  } else if (next_gcode_idx_ >= gcode_list_.size()) {
    return 100;
  }
  return 100 * (float)(next_gcode_idx_) / gcode_list_.size();
}

Timestamp RotaryTestJob::getElapsedTime() const {
  return Timestamp();
}

Timestamp RotaryTestJob::getTotalRequiredTime() const {
  return Timestamp();
}

Timestamp RotaryTestJob::getRemainingTime() const {
  return Timestamp();
}
