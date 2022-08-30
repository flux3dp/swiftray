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
  MachineSettings::MachineSet m;
  m.origin = MachineSettings::MachineSet::OriginType::RearLeft;
  m.board_type = MachineSettings::MachineSet::BoardType::GRBL_2020;
  motion_controller_ = MotionControllerFactory::createMotionController(m, this);
  //motion_controller_ = MotionControllerFactory::createMotionController(machine_settings_.board_type, this);
  
  //QThread *rt_status_thread = new QThread;
  //MachineRTStatusUpdater *rt_status_updater = new MachineRTStatusUpdater();
  //rt_status_updater->moveToThread(rt_status_thread);
  //connect(rt_status_updater, &MachineRTStatusUpdater::error, this, &Machine::errorString);
  //connect(rt_status_thread, &QThread::started, rt_status_updater, &MachineRTStatusUpdater::process);
  //connect(rt_status_updater, &MachineRTStatusUpdater::finished, rt_status_thread, &QThread::quit);
  //connect(rt_status_updater, &MachineRTStatusUpdater::finished, rt_status_updater, &MachineRTStatusUpdater::deleteLater);
  //connect(rt_status_thread, &QThread::finished, rt_status_thread, &QThread::deleteLater);
  //rt_status_thread->start();

}

bool Machine::setNewJob(MachineJob *new_job) {
  
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
  // TODO: Use general Port class instead of SerialPort class
  SerialPort* port = qobject_cast<SerialPort*>(sender());
  if (port == nullptr) {
    return;
  }
  motion_controller_->attachPort(port);
  connect(motion_controller_, &MotionController::disconnected, this, &Machine::motionPortDisonnected);
  
  // TODO: Create only when nullptr? In case executor already exists
  job_executor_ = new JobExecutor{motion_controller_, this};
  machine_setup_executor_ = new MachineSetupExecutor{motion_controller_, this};
  rt_status_executor_ = new RTStatusUpdateExecutor{motion_controller_, this};
  connect(machine_setup_executor_, &Executor::finished, this, &Machine::motionPortActivated);
  machine_setup_executor_->start();
}

void Machine::motionPortActivated() {
  connect(rt_status_executor_, &RTStatusUpdateExecutor::hanging, [=]() {
    // TODO: handle machine not responding to realtime status cmd
  });
  job_executor_->start();
  rt_status_executor_->start();
}

void Machine::motionPortDisonnected() {
  qInfo() << "Machine::motionPortDisonnected()";
  job_executor_->deleteLater();
  machine_setup_executor_->deleteLater();
  rt_status_executor_->deleteLater();
  //current_job_;
}
