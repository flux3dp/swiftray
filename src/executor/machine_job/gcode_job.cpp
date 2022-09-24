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

GCodeJob::GCodeJob(QStringList gcode_list, QPixmap preview, QString job_name) 
  : MachineJob{job_name}
{
  gcode_list_ = gcode_list;
  preview_ = preview;
  with_preview_ = true;
}

GCodeJob::GCodeJob(QString gcodes, QPixmap preview, QString job_name) 
  : MachineJob{job_name}
{
  gcode_list_ = gcodes.split('\n');
  preview_ = preview;
  with_preview_ = true;
}

void GCodeJob::setMotionController(QPointer<MotionController> motion_controller) {
  motion_controller_ = motion_controller;
}

void GCodeJob::setTimestampList(const QList<Timestamp> &ts_list) {
  timestamp_list_ = ts_list;
}

void GCodeJob::setTimestampList(QList<Timestamp> &&ts_list) {
  timestamp_list_ = ts_list;
}

std::shared_ptr<OperationCmd> GCodeJob::getNextCmd() {
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

bool GCodeJob::end() const {
  return next_gcode_idx_ >= gcode_list_.length();
}

void GCodeJob::reload() {
  next_gcode_idx_ = 0;
}

float GCodeJob::getProgressPercent() const {
  if (getTotalRequiredTime().totalMSecs() == 0) {
    return 100.0;
  }
  return ((float)(getElapsedTime().totalMSecs()) / getTotalRequiredTime().totalMSecs()) * 100;
}

Timestamp GCodeJob::getElapsedTime() const {
  if (timestamp_list_.isEmpty()) {
    return Timestamp();
  } else if (next_gcode_idx_ == 0) { 
    return Timestamp();
  } else if ((next_gcode_idx_ - 1) < timestamp_list_.size()) {
    return timestamp_list_.at(next_gcode_idx_ - 1);
  } else {
    return timestamp_list_.last();
  }
}

Timestamp GCodeJob::getTotalRequiredTime() const {
  if (timestamp_list_.isEmpty()) {
    return Timestamp();
  }
  return timestamp_list_.last();
}

Timestamp GCodeJob::getRemainingTime() const {
  if (timestamp_list_.isEmpty()) {
    return Timestamp();
  } else if (next_gcode_idx_ == 0) { 
    return Timestamp();
  } else if ((next_gcode_idx_ - 1) < timestamp_list_.size()) {
    return Timestamp().addSecs(timestamp_list_.at(next_gcode_idx_ - 1).secsTo(timestamp_list_.last()));
  } else {
    return Timestamp();
  }
}
