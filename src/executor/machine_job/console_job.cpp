#include "console_job.h"

#include "../operation_cmd/gcode_cmd.h"
#include <string>S

ConsoleJob::ConsoleJob(QString command, QString job_name) 
  : MachineJob{job_name}
{
  gcode_list_.push_back(command);
}

void ConsoleJob::setMotionController(QPointer<MotionController> motion_controller) {
  motion_controller_ = motion_controller;
}

std::shared_ptr<OperationCmd> ConsoleJob::getNextCmd() {
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

bool ConsoleJob::end() const {
  if (next_gcode_idx_ >= gcode_list_.size()) {
    return true;
  }
  return false;
}

void ConsoleJob::reload() {
  next_gcode_idx_ = 0;
}

float ConsoleJob::getProgressPercent() const {
  if (gcode_list_.isEmpty()) {
    return 100;
  } else if (next_gcode_idx_ >= gcode_list_.size()) {
    return 100;
  }
  return 100 * (float)(next_gcode_idx_) / gcode_list_.size();
}

Timestamp ConsoleJob::getElapsedTime() const {
  return Timestamp();
}

Timestamp ConsoleJob::getTotalRequiredTime() const {
  return Timestamp();
}

Timestamp ConsoleJob::getRemainingTime() const {
  return Timestamp();
}
