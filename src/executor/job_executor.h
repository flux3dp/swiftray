#ifndef JOBEXECUTOR_H
#define JOBEXECUTOR_H

#include <QObject>
#include "executor.h"
#include <motion_controller/motion_controller.h>
#include <machine_job/machine_job.h>
#include <QPointer>
#include <QSharedPointer>
#include <mutex>

class JobExecutor : public Executor
{
public:
  explicit JobExecutor(QPointer<MotionController> motion_controller, QObject *parent = nullptr);

  bool setNewJob(QSharedPointer<MachineJob> new_job);

public slots:
  void exec() override;

  void onCmdAcked();

private:
  bool idle(); // All acked cmds have finished execution

  QPointer<MotionController> motion_controller_;
  QSharedPointer<MachineJob> active_job_; // current running job
  QSharedPointer<MachineJob> pending_job_;// The next job to be activated
  QSharedPointer<MachineJob> last_job_;   // When finished, move active_job_ to here for replay later
  bool error_occurred_ = false;
  size_t acquired_cmd_cnt_ = 0; // acquired_cmd_cnt_ >= sent_cmd_cnt_ >= acked_cmd_cnt_
  size_t sent_cmd_cnt_ = 0;
  size_t acked_cmd_cnt_ = 0;  // Updated in slot
  std::mutex acked_cmd_cnt_mutex_;

};

#endif // JOBEXECUTOR_H
