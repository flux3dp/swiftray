#include "framing_job.h"

FramingJob::FramingJob(QString job_name, QObject *parent)
  :MachineJob{job_name, parent}
{

}

bool FramingJob::isActive() {
  return true;
}

std::tuple<Target, QString> FramingJob::getNextCmd() {
  return std::make_tuple(Target::kMotionControl, "");
}

bool FramingJob::end() {
  return true;
}
