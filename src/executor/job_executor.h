#ifndef JOBEXECUTOR_H
#define JOBEXECUTOR_H

#include <QObject>
#include "executor.h"
#include <periph/motion_controller/motion_controller.h>
#include "machine_job/machine_job.h"
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
  void setRepeat(size_t repeat);
  size_t getRepeat();


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
  size_t repeat_ = 1; // remaining repeat count
};

#endif // JOBEXECUTOR_H
