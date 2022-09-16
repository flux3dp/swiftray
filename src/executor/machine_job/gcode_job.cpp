#include "gcode_job.h"

#include "../operation_cmd/gcode_cmd.h"

GCodeJob::GCodeJob(QStringList gcode_list, QString job_name)
  : MachineJob{job_name}
{
  gcode_list_ = gcode_list;
}

GCodeJob::GCodeJob(QString gcodes, QString job_name)
  : MachineJob{job_name}
{
  gcode_list_ = gcodes.split('\n');
}

void GCodeJob::setMotionController(QPointer<MotionController> motion_controller) {
  motion_controller_ = motion_controller;
}

std::shared_ptr<OperationCmd> GCodeJob::getNextCmd() {
  if (end()) {
    return std::shared_ptr<OperationCmd>{nullptr};
  }
  auto idx = current_gcode_idx_++;
  auto cmd = std::make_shared<GCodeCmd>();
  cmd->setGCode(gcode_list_.at(idx) + "\n");
  //cmd->setTarget(OperationCmd::Target::kMotionControl);
  cmd->setMotionController(motion_controller_);
  return cmd;
}

bool GCodeJob::end() {
  return current_gcode_idx_ >= gcode_list_.length();
}

bool GCodeJob::isActive() {
  return current_gcode_idx_ > 0 && current_gcode_idx_ < gcode_list_.length();
}

void GCodeJob::reload() {
  current_gcode_idx_ = 0;
}
