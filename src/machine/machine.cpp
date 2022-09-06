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

  // Create executors belong to the machine
  job_executor_ = new JobExecutor{motion_controller_};
  machine_setup_executor_ = new MachineSetupExecutor{this};
  rt_status_executor_ = new RTStatusUpdateExecutor{this};

  connect(rt_status_executor_, &RTStatusUpdateExecutor::hanging, [=]() {
    // TODO: 
    qInfo() << "Realtime status updater hanging";
  });
  connect(machine_setup_executor_, &Executor::finished, this, &Machine::motionPortActivated);
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
  if (motion_controller_) {
    motion_controller_->deleteLater();
    motion_controller_ = nullptr;
  }
  motion_controller_ = MotionControllerFactory::createMotionController(machine_param_, this);
  connect(motion_controller_, &MotionController::disconnected, this, &Machine::motionPortDisonnected);
  motion_controller_->attachPort(port);
  
  // Attach motion_controller to executors
  rt_status_executor_->attachMotionController(motion_controller_);
  machine_setup_executor_->attachMotionController(motion_controller_);
  job_executor_->attachMotionController(motion_controller_);

  machine_setup_executor_->start();
}

void Machine::motionPortActivated() {
  rt_status_executor_->start();
}

void Machine::motionPortDisonnected() {
  // ...
  qInfo() << "Machine::motionPortDisonnected()";
}

void Machine::testJobStart() {
  QStringList cmd_list;
  cmd_list << "$X";
  cmd_list << "G1F3000";
  cmd_list << "G1X50";
  cmd_list << "G1Y50";
  cmd_list << "G1X0";
  cmd_list << "G1Y0";
  current_job_ = QSharedPointer<GCodeJob>::create(cmd_list);
  job_executor_->setNewJob(current_job_);
  job_executor_->start();
}
