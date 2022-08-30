#include "job_executor.h"

#include <QDebug>

JobExecutor::JobExecutor(MotionController *motion_controller, QObject *parent)
  : Executor{parent}, motion_controller_{motion_controller}
{
  qInfo() << "JobExecutor created";
}

void JobExecutor::start() {
  qInfo() << "JobExecutor start()";
}

bool JobExecutor::setNewJob(MachineJob *new_job) {
  
  return true;
}
