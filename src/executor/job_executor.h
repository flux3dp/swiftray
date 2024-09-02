#pragma once

#include <QObject>
#include "executor.h"
#include "machine_job/machine_job.h"
#include <common/timestamp.h>
#include <QPointer>
#include <QSharedPointer>
#include <mutex>
#include <QTimer>
#include <atomic>
#include <thread>

class JobExecutor : public Executor
{
  Q_OBJECT
public:
  explicit JobExecutor(QObject *parent = nullptr);
  bool setNewJob(QSharedPointer<MachineJob> new_job);
  MachineJob const *getActiveJob() const;
  Timestamp getTotalRequiredTime() const;
  float getProgress() const;
  Timestamp getElapsedTime() const;
  void reset();
  void startJob();
  void handleCmdFinish(int result_code) override;

private Q_SLOTS:
  void handlePaused() override;
  void handleResume() override;
  void handleStopped() override;
  void exec() override;
  void handleMotionControllerStatusUpdate(MotionControllerState mc_state, qreal x_pos, qreal y_pos, qreal z_pos) override;

Q_SIGNALS:
  void progressChanged(float prog);
  void elapsedTimeChanged(Timestamp);

protected:
  void threadFunction();

private:
  std::mutex exec_mutex_;

  QSharedPointer<MachineJob> active_job_; // current running job
  QSharedPointer<MachineJob> pending_job_;// The next job to be activated
  QSharedPointer<MachineJob> last_job_;   // When finished, move active_job_ to here for replay later

  QTimer *exec_timer_;
  std::shared_ptr<OperationCmd> pending_cmd_;
  MotionControllerState latest_mc_state_;
  size_t completed_cmd_cnt_ = 0;
  bool running_{false};
};