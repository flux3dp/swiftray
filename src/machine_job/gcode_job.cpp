#include "gcode_job.h"

GCodeJob::GCodeJob(QStringList gcode_list, QString job_name, QObject *parent)
  : MachineJob{job_name, parent}
{
  gcode_list_ = gcode_list;
}

GCodeJob::GCodeJob(QString gcodes, QString job_name, QObject *parent)
  : MachineJob{job_name, parent}
{
  gcode_list_ = gcodes.split('\n');
}

std::tuple<Target, QString> GCodeJob::getNextCmd() {
  if (end()) {
    return std::make_tuple(Target::kMotionControl, "");
  }
  return std::make_tuple(Target::kMotionControl, gcode_list_.at(current_gcode_idx_));
}

bool GCodeJob::end() {
  return current_gcode_idx_ >= gcode_list_.length();
}

bool GCodeJob::isActive() {
  return current_gcode_idx_ > 0 && current_gcode_idx_ < gcode_list_.length();
}