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