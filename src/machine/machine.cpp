#include "machine.h"
#include <QThread>

#include <settings/machine-settings.h>
#include <motion_controller/motion_controller_factory.h>

Machine::Machine(QObject *parent)
  : QObject{parent}
{

  // TODO: Pass MachineSettings::MachineSet as argument
  //       Should refactor/restructure MachineSettings beforehand
  //       The followings is just a placeholder
  machine_param_.origin = MachineSettings::MachineSet::OriginType::RearLeft;
  machine_param_.board_type = MachineSettings::MachineSet::BoardType::GRBL_2020;
  //motion_controller_ = MotionControllerFactory::createMotionController(m, this);

}

bool Machine::setNewJob(QSharedPointer<MachineJob> new_job) {
  if (!job_executor_) {
    return false;
  }
  if (!job_executor_->setNewJob(new_job)) {
    return false;
  }
  current_job_ = new_job;

  return true;
}
/*
bool Machine::setMachineSettings(MachineSettings::MachineSet machine_settings) {
  // Block if machine is connected
  if (motion_controller_.isConnected()) {
    return false;
  }
  // TODO: Block if other controllers (e.g. camera) are connected?

  machine_settings_ = machine_settings;
  motion_controller_ = MotionControllerFactory::createMotionController(machine_settings_.board_type, this);
  return true;
}
*/

void Machine::motionPortConnected() {
  qInfo() << "Machine::motionPortConnected()";
  // TODO: Use general Port class instead of SerialPort class
  SerialPort* port = qobject_cast<SerialPort*>(sender());
  if (port == nullptr) {
    return;
  }

  // TODO: Pass MachineSettings::MachineSet as argument
  //       Should refactor/restructure MachineSettings beforehand
  //       The followings is just a placeholder
  motion_controller_ = MotionControllerFactory::createMotionController(machine_param_, this);
  
  motion_controller_->attachPort(port);
  connect(motion_controller_, &MotionController::disconnected, this, &Machine::motionPortDisonnected);
  
  // TODO: Create only when nullptr? In case executor already exists
  //job_executor_ = new JobExecutor{motion_controller_};
  //job_exec_thread_ = new QThread;
  //job_executor_->moveToThread(job_exec_thread_);
  //connect(job_exec_thread_, &QThread::started, job_executor_, &JobExecutor::start);
  //connect(job_executor_, &Executor::finished, job_exec_thread_, &QThread::quit);

  machine_setup_executor_ = new MachineSetupExecutor{motion_controller_};
  machine_setup_exec_thread_ = new QThread;
  machine_setup_executor_->moveToThread(machine_setup_exec_thread_);
  connect(machine_setup_exec_thread_, &QThread::started, machine_setup_executor_, &MachineSetupExecutor::start);
  connect(machine_setup_executor_, &Executor::finished, machine_setup_exec_thread_, &QThread::quit);
  connect(machine_setup_executor_, &Executor::finished, machine_setup_executor_, &MachineSetupExecutor::deleteLater);
  connect(machine_setup_exec_thread_, &QThread::finished, machine_setup_exec_thread_,  &QThread::deleteLater);

  rt_status_executor_ = new RTStatusUpdateExecutor{motion_controller_};
  rt_status_exec_thread_ = new QThread;
  rt_status_executor_->moveToThread(rt_status_exec_thread_);
  connect(rt_status_exec_thread_, &QThread::started, rt_status_executor_, &RTStatusUpdateExecutor::start);
  connect(rt_status_executor_, &Executor::finished, rt_status_exec_thread_, &QThread::quit);
  connect(rt_status_executor_, &Executor::finished, rt_status_executor_, &RTStatusUpdateExecutor::deleteLater);
  connect(rt_status_exec_thread_, &QThread::finished, rt_status_exec_thread_,  &QThread::deleteLater);

  connect(machine_setup_executor_, &Executor::finished, this, &Machine::motionPortActivated);
  machine_setup_exec_thread_->start();
}

void Machine::motionPortActivated() {
  connect(rt_status_executor_, &RTStatusUpdateExecutor::hanging, [=]() {
    // TODO: handle machine not responding to realtime status cmd
    qInfo() << "Realtime status updater hanging";
  });
  //job_exec_thread_->start();
  rt_status_exec_thread_->start();
}

void Machine::motionPortDisonnected() {
  motion_controller_->deleteLater();
  motion_controller_ = nullptr;
  //qInfo() << "Machine::motionPortDisonnected()";
  //qInfo() << "rt_status_exec_thread_ state: ";
  //qInfo() << rt_status_exec_thread_->isFinished();
  //qInfo() << rt_status_exec_thread_->isInterruptionRequested();
  //qInfo() << rt_status_exec_thread_->isRunning();

  //job_executor_->deleteLater();
  //machine_setup_executor_->deleteLater();
  //rt_status_executor_->deleteLater();
  //job_exec_thread_->deleteLater();
  //machine_setup_exec_thread_->deleteLater();
  //rt_status_exec_thread_->deleteLater();
  //job_executor_ = nullptr;
  //machine_setup_executor_ = nullptr;
  //rt_status_executor_ = nullptr;
  //job_exec_thread_ = nullptr;
  //machine_setup_exec_thread_ = nullptr;
  //rt_status_exec_thread_ = nullptr;
  //current_job_; ?
}
