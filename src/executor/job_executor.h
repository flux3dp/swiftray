#ifndef JOBEXECUTOR_H
#define JOBEXECUTOR_H

#include <QObject>
#include "executor.h"
#include <periph/motion_controller/motion_controller.h>
#include "machine_job/machine_job.h"
#include <common/timestamp.h>
#include <QPointer>
#include <QSharedPointer>
#include <mutex>
#include <QTimer>

class JobExecutor : public Executor
{
  Q_OBJECT
public:
  explicit JobExecutor(QObject *parent = nullptr);
  void handleCmdFinish(int result_code) override;

  bool setNewJob(QSharedPointer<MachineJob> new_job);
  void attachMotionController(QPointer<MotionController> motion_controller);
  MachineJob const *getActiveJob() const;
  Timestamp getTotalRequiredTime() const;
  float getProgress() const;
  Timestamp getElapsedTime() const;


public slots:
  void start() override;
  void exec() override;
  void pause() override;
  void resume() override;
  void stop() override;

private slots:
  void wakeUp();

  //void onCmdAcked();
signals:
  void trigger();
  void progressChanged(float prog);
  void elapsedTimeChanged(Timestamp);

private:
  void complete();
  void stopImpl();
  std::mutex exec_mutex_;

  QPointer<MotionController> motion_controller_;
  QSharedPointer<MachineJob> active_job_; // current running job
  QSharedPointer<MachineJob> pending_job_;// The next job to be activated
  QSharedPointer<MachineJob> last_job_;   // When finished, move active_job_ to here for replay later

  QTimer *exec_timer_;
  std::shared_ptr<OperationCmd> pending_cmd_;
  MotionControllerState latest_mc_state_;
  size_t completed_cmd_cnt_ = 0;
};

#endif // JOBEXECUTOR_H
