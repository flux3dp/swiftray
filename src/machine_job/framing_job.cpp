#include "framing_job.h"

FramingJob::FramingJob(QString job_name, QObject *parent)
  :MachineJob{job_name, parent}
{

}

bool FramingJob::isActive() {
  // TODO: 
  return true;
}

std::tuple<Target, QString> FramingJob::getNextCmd() {
  // TODO: 
  return std::make_tuple(Target::kMotionControl, "");
}

bool FramingJob::end() {
  // TODO: 
  return true;
}

void FramingJob::reload() {
  // TODO:

}
