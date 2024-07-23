#include "jogging_job.h"

#include "../operation_cmd/gcode_cmd.h"

JoggingRelativeJob::JoggingRelativeJob(qreal x_pos, qreal y_pos, qreal feedrate, QString job_name)
  : MachineJob{job_name}
{
  // Populate gcode list
  gcode_list_.push_back("G91");
  QString move_gc = "G1" "F" + QString::number(feedrate)
                  + "X" + QString::number(x_pos)
                  + "Y" + QString::number(y_pos)
                  + "S0";
  gcode_list_.push_back(move_gc);
  gcode_list_.push_back("G90");
  gcode_list_.push_back("M2"); // Sync program flow and End the program (clear state: turn off laser, turn off coolant, ...)
  gcode_list_.push_back("?");  // Get the realtime position right at the end of job
}

void JoggingRelativeJob::setMotionController(QPointer<MotionController> motion_controller) {
  motion_controller_ = motion_controller;
}

std::shared_ptr<OperationCmd> JoggingRelativeJob::getNextCmd() {
  if (end()) {
    return std::shared_ptr<OperationCmd>{nullptr};
  }
  auto idx = next_gcode_idx_++;
  auto cmd = std::make_shared<GCodeCmd>(gcode_list_.at(idx) + "\n");
  return cmd;
}

bool JoggingRelativeJob::end() const {
  if (next_gcode_idx_ >= gcode_list_.size()) {
    return true;
  }
  return false;
}

void JoggingRelativeJob::reload() {
  next_gcode_idx_ = 0;
}

float JoggingRelativeJob::getProgressPercent() const {
  if (gcode_list_.isEmpty()) {
    return 100;
  } else if (next_gcode_idx_ >= gcode_list_.size()) {
    return 100;
  }
  return 100 * (float)(next_gcode_idx_) / gcode_list_.size();
}

Timestamp JoggingRelativeJob::getElapsedTime() const {
  return Timestamp();
}

Timestamp JoggingRelativeJob::getTotalRequiredTime() const {
  return Timestamp();
}

Timestamp JoggingRelativeJob::getRemainingTime() const {
  return Timestamp();
}


JoggingAbsoluteJob::JoggingAbsoluteJob(bool x_exist, qreal x_pos, bool y_exist, qreal y_pos, 
                                      qreal feedrate, QString job_name)
  : MachineJob{job_name}
{
  // Populate gcode list
  gcode_list_.push_back("G90");
  QString move_gc = "G1";
  move_gc += "F" + QString::number(feedrate);
  if (x_exist) {
    move_gc += "X" + QString::number(x_pos);
  } 
  if (y_exist) {
    move_gc += "Y" + QString::number(y_pos);
  }
  move_gc += "S0";
  gcode_list_.push_back(move_gc);
  gcode_list_.push_back("M2"); // Sync program flow and End the program (clear state: turn off laser, turn off coolant, ...)
  gcode_list_.push_back("?");  // Get the realtime position right at the end of job
}

void JoggingAbsoluteJob::setMotionController(QPointer<MotionController> motion_controller) {
  motion_controller_ = motion_controller;
}

std::shared_ptr<OperationCmd> JoggingAbsoluteJob::getNextCmd() {
  if (end()) {
    return std::shared_ptr<OperationCmd>{nullptr};
  }
  auto idx = next_gcode_idx_++;
  auto cmd = std::make_shared<GCodeCmd>(gcode_list_.at(idx) + "\n");
  return cmd;
}

bool JoggingAbsoluteJob::end() const {
  if (next_gcode_idx_ >= gcode_list_.size()) {
    return true;
  }
  return false;
}

void JoggingAbsoluteJob::reload() {
  next_gcode_idx_ = 0;
}

float JoggingAbsoluteJob::getProgressPercent() const {
  if (gcode_list_.isEmpty()) {
    return 100;
  } else if (next_gcode_idx_ >= gcode_list_.size()) {
    return 100;
  }
  return 100 * (float)(next_gcode_idx_) / gcode_list_.size();
}

Timestamp JoggingAbsoluteJob::getElapsedTime() const {
  return Timestamp();
}

Timestamp JoggingAbsoluteJob::getTotalRequiredTime() const {
  return Timestamp();
}

Timestamp JoggingAbsoluteJob::getRemainingTime() const {
  return Timestamp();
}
