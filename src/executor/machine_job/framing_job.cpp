#include "framing_job.h"

#include "../operation_cmd/gcode_cmd.h"

FramingJob::FramingJob(QStringList gcode_list, QString job_name) 
  : MachineJob{job_name}
{
  gcode_list_ = gcode_list;
}

FramingJob::FramingJob(QString gcodes, QString job_name)
  : MachineJob{job_name}
{
  gcode_list_ = gcodes.split('\n');
}