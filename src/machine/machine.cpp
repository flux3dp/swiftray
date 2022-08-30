#include "machine.h"
#include <QThread>

#include <motion_controller/motion_controller_factory.h>

Machine::Machine(QObject *parent)
  : QObject{parent}
{

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

bool Machine::setNewJob(QSharedPointer<MachineJob> new_job) {
  
  if (!job_executor_->setNewJob(new_job)) {
    return false
  }
  current_job_ = new_job;

  return true;
}

bool Machine::setMachineSettings(MachineSettings::MachineSet machine_settings) {
  // Block if machine is connected
  if (motion_controller_.isConnected()) {
    return false;
  }
  // TODO: Block if other controllers (e.g. camera) are connected?

  machine_settings_ = machine_settings;
  motion_controller_ = MotionControllerFactory::getMotionController(machine_settings_.board_tupe);
  return true;
}