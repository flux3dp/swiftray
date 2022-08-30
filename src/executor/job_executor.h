#ifndef JOBEXECUTOR_H
#define JOBEXECUTOR_H

#include <QObject>
#include "executor.h"
#include <motion_controller/motion_controller.h>
#include <machine_job/machine_job.h>

class JobExecutor : public Executor
{
public:
  explicit JobExecutor(MotionController *motion_controller, QObject *parent = nullptr);

  void start() override;

  bool setNewJob(MachineJob *new_job);

private:
  MotionController *motion_controller_;
};

#endif // JOBEXECUTOR_H
