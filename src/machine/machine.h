#ifndef MACHINE_H
#define MACHINE_H

#include <QObject>
#include <QSharedPointer>
#include <QPointer>
#include <QThread>
#include <QStringList>
#include <QProgressDialog>
#include <QPixmap>
#include <connection/serial-port.h>
#include <settings/machine-settings.h>
#include <periph/motion_controller/motion_controller.h>
#include <executor/machine_job/machine_job.h>
#include <executor/machine_setup_executor.h>
#include <executor/job_executor.h>
#include <executor/rt_status_update_executor.h>
#include <executor/console_executor.h>

class Machine : public QObject
{
  Q_OBJECT
public:
  explicit Machine(QObject *parent = nullptr);

  void applyMachineParam(MachineSettings::MachineSet mach);
  bool createGCodeJob(QStringList gcode_list, QPointer<QProgressDialog> progress_dialog);
  bool createGCodeJob(QStringList gcode_list, QPixmap preview, QPointer<QProgressDialog> progress_dialog);
  bool createFramingJob(QStringList gcode_list);
  //QSharedPointer<MachineJob> getCurrentJob() const { return current_job_; }
  QPointer<MotionController> getMotionController() const { return motion_controller_; }
  QPointer<JobExecutor> getJobExecutor() const { return job_executor_; }
  QPointer<RTStatusUpdateExecutor> getRTSatatusUpdateExecutor() const { return rt_status_executor_; }
  
  //bool setMachineSettings(MachineSettings::MachineSet machine_settings);
  //MachineSettings::MachineSet getMachineSettings() const { return machine_settings_; }

public slots:
  void motionPortConnected();  // Opened but not check
  void motionPortActivated();  // Motion controller working
  void motionPortDisonnected();// Closed

  void startJob();
  void pauseJob();
  void resumeJob();
  void stopJob();

private:
  MachineSettings::MachineSet machine_param_; // Settings for software, NOT the grbl settings

  // Hardware equipment controllers
  MotionController *motion_controller_;
  //AutofocusController *af_controller_;
  //CameraController *camera_controller_;
  
  // Task Executors
  JobExecutor *job_executor_;
  ConsoleExecutor *console_executor_; // A general-purpose simple cmd executor
  MachineSetupExecutor *machine_setup_executor_;
  RTStatusUpdateExecutor *rt_status_executor_;
  //QThread *job_exec_thread_;
  //QThread *machine_setup_exec_thread_;
  //QThread *rt_status_exec_thread_;
  //UserCmdExecutor *user_cmd_executor_;
  //JoggingExecutor *jogging_executor_;
};

#endif // MACHINE_H
