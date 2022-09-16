#include "framing_job.h"

#include "../operation_cmd/gcode_cmd.h"

FramingJob::FramingJob(QString job_name)
  :MachineJob{job_name}
{

}

bool FramingJob::isActive() {
  // TODO: 
  return true;
}

std::shared_ptr<OperationCmd> FramingJob::getNextCmd() {
  // TODO: 
  //return std::make_shared<GCodeCmd>();
  std::shared_ptr<OperationCmd> cmd;
  return cmd;
}

bool FramingJob::end() {
  // TODO: 
  return true;
}

void FramingJob::reload() {
  // TODO:

}
