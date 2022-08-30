#ifndef MACHINE_H
#define MACHINE_H

#include <QObject>
#include <QSharedPointer>
#include <QWeakPointer>
#include <connection/serial-port.h>
#include <settings/machine-settings.h>
#include <motion_controller/motion-controller.h>

class Machine : public QObject
{
  Q_OBJECT
public:
  explicit Machine(QObject *parent = nullptr);

  bool setNewJob(QSharedPointer<MachineJob> new_job);
  QWeakPointer<MachineJob> getCurrentJob() const { return current_job_.toWeakRef(); }
  QWeakPointer<MotionController> getMotionController() const { return motion_controller_.toWeakRef(); }
  QWeakPointer<JobExecutor> getJobExecutor() const { return job_executor_.toWeakRef(); }
  QWeakPointer<RTSatatusUpdateExecutor> getRTSatatusUpdateExecutor() const { return rt_status_executor_.toWeakRef(); }
  
  bool setMachineSettings(MachineSettings::MachineSet machine_settings);
  MachineSettings::MachineSet getMachineSettings() const { return machine_settings_; }

private:
  MachineSettings::MachineSet machine_settings_; // Settings for software, NOT the grbl settings

  // Hardware equipment controllers
  QSharedPointer<MotionController> motion_controller_;
  //QSharedPointer<AutofocusController> af_controller_;
  //QSharedPointer<CameraController> camera_controller_;
  
  // Task Executors
  QSharedPointer<MachineJob> current_job_;
  QSharedPointer<JobExecutor> job_executor_;
  QSharedPointer<MachineSetupExecutor> machine_setup_executor_;
  QSharedPointer<RTStatusUpdateExecutor> rt_status_executor_;
  //QSharedPointer<UserCmdExecutor> user_cmd_executor_;
  //QSharedPointer<JoggingExecutor> jogging_executor_;
};

#endif // MACHINE_H
