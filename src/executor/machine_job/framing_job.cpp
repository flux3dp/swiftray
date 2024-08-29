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