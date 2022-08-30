#include "machine_job.h"

MachineJob::MachineJob(QString job_name, QObject *parent)
  : QObject{parent}
{
  job_name_ = job_name;
}

void MachineJob::setRepeat(uint32_t repeat) { 
  std::lock_guard<std::mutex> lk(repeat_mutex_);
  repeat_ = repeat;
}

uint32_t MachineJob::getRepeat() {
  std::lock_guard<std::mutex> lk(repeat_mutex_);
  return repeat_; 
};
