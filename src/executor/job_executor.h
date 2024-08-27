#pragma once

#include <QObject>
#include "executor.h"
#include <periph/motion_controller/motion_controller.h>
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
  ~JobExecutor();

  bool setNewJob(QSharedPointer<MachineJob> new_job);
  void attachMotionController(QPointer<MotionController> motion_controller);
  MachineJob const *getActiveJob() const;
  Timestamp getTotalRequiredTime() const;
  float getProgress() const;
  Timestamp getElapsedTime() const;
  void reset();
  void start() override;
  void handleCmdFinish(int result_code) override;

private Q_SLOTS:
  void handlePaused() override;
  void handleResume() override;
  void handleStopped() override;
  void exec() override;
  void handleDisconnect();
  void handleMotionControllerStateChanged(MotionControllerState last_state, MotionControllerState current_state, 
                                  qreal x_pos, qreal y_pos, qreal z_pos);

Q_SIGNALS:
  void progressChanged(float prog);
  void elapsedTimeChanged(Timestamp);

protected:
  void threadFunction();

private:
  std::mutex exec_mutex_;

  QPointer<MotionController> motion_controller_;
  QSharedPointer<MachineJob> active_job_; // current running job
  QSharedPointer<MachineJob> pending_job_;// The next job to be activated
  QSharedPointer<MachineJob> last_job_;   // When finished, move active_job_ to here for replay later

  QTimer *exec_timer_;
  std::shared_ptr<OperationCmd> pending_cmd_;
  MotionControllerState latest_mc_state_;
  size_t completed_cmd_cnt_ = 0;
  bool running_{false};

  std::thread exec_thread_;
};