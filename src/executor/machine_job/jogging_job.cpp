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