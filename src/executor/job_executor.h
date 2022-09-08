#ifndef JOBEXECUTOR_H
#define JOBEXECUTOR_H

#include <QObject>
#include "executor.h"
#include <motion_controller/motion_controller.h>
#include <machine_job/machine_job.h>
#include <QPointer>
#include <QSharedPointer>
#include <mutex>
#include <QTimer>

class JobExecutor : public Executor
{
public:
  explicit JobExecutor(QObject *parent = nullptr);

  bool setNewJob(QSharedPointer<MachineJob> new_job);
  void attachMotionController(QPointer<MotionController> motion_controller);

public slots:
  void start() override;
  void exec() override;
  void pause() override;
  void resume() override;
  void stop() override;

  void onCmdAcked();

private:
  enum class State {
    kIdle,
    kActive,
    kPaused
  };

  QPointer<MotionController> motion_controller_;
  QSharedPointer<MachineJob> active_job_; // current running job
  QSharedPointer<MachineJob> pending_job_;// The next job to be activated
  QSharedPointer<MachineJob> last_job_;   // When finished, move active_job_ to here for replay later
  bool error_occurred_ = false;
  size_t sent_cmd_cnt_ = 0;
  size_t acked_cmd_cnt_ = 0;  // Updated in slot
  std::mutex acked_cmd_cnt_mutex_;
  bool finishing_ = false;
  bool current_cmd_is_sent_ = true;
  bool block_until_all_acked_and_idle_ = false;
  std::tuple<Target, QString> current_cmd = std::make_tuple(Target::kNone, ""); // cmd to be sent

  QTimer *exec_timer_;

  State state_ = State::kIdle;
};

#endif // JOBEXECUTOR_H
