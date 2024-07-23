#include "framing_job.h"

#include "../operation_cmd/gcode_cmd.h"

FramingJob::FramingJob(QStringList gcode_list, QString job_name) 
  : MachineJob{job_name}
{
  gcode_list_ = gcode_list;
  gcode_list_.push_back("?");  // Get the realtime position right at the end of job
}

FramingJob::FramingJob(QString gcodes, QString job_name)
  : MachineJob{job_name}
{
  gcode_list_ = gcodes.split('\n');
  gcode_list_.push_back("?");  // Get the realtime position right at the end of job
}

void FramingJob::setMotionController(QPointer<MotionController> motion_controller) {
  motion_controller_ = motion_controller;
}

std::shared_ptr<OperationCmd> FramingJob::getNextCmd() {
  if (end()) {
    return std::shared_ptr<OperationCmd>{nullptr};
  }
  auto idx = next_gcode_idx_++;
  auto cmd = std::make_shared<GCodeCmd>(gcode_list_.at(idx) + "\n");
  return cmd;
}

bool FramingJob::end() const {
  if (next_gcode_idx_ >= gcode_list_.size()) {
    return true;
  }
  return false;
}

void FramingJob::reload() {
  next_gcode_idx_ = 0;
}

float FramingJob::getProgressPercent() const {
  if (gcode_list_.isEmpty()) {
    return 100;
  } else if (next_gcode_idx_ >= gcode_list_.size()) {
    return 100;
  }
  return 100 * (float)(next_gcode_idx_) / gcode_list_.size();
}

Timestamp FramingJob::getElapsedTime() const {
  return Timestamp();
}

Timestamp FramingJob::getTotalRequiredTime() const {
  return Timestamp();
}

Timestamp FramingJob::getRemainingTime() const {
  return Timestamp();
}
