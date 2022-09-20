#include "machine.h"
//#include <QThread>
#include <QElapsedTimer>
#include <QtMath>

#include <settings/machine-settings.h>
#include <periph/motion_controller/motion_controller_factory.h>
#include <executor/machine_job/gcode_job.h>
#include <executor/machine_job/framing_job.h>
#include <executor/operation_cmd/grbl_cmd.h>

Machine::Machine(QObject *parent)
  : QObject{parent}
{

  // Apply a default (placeholder) machine param
  MachineSettings::MachineSet mach;
  mach.origin = MachineSettings::MachineSet::OriginType::RearLeft;
  mach.board_type = MachineSettings::MachineSet::BoardType::GRBL_2020;
  applyMachineParam(mach);

  // Create executors belong to the machine
  job_executor_ = new JobExecutor{motion_controller_};
  machine_setup_executor_ = new MachineSetupExecutor{this};
  rt_status_executor_ = new RTStatusUpdateExecutor{this};
  console_executor_ = new ConsoleExecutor(motion_controller_);

  connect(rt_status_executor_, &RTStatusUpdateExecutor::hanging, [=]() {
    // TODO: 
    qInfo() << "Realtime status updater hanging";
  });
  connect(machine_setup_executor_, &Executor::finished, this, &Machine::motionPortActivated);
}

/**
 * @brief Apply (change) machine params to this machine controller
 * 
 * @param mach 
 */
void Machine::applyMachineParam(MachineSettings::MachineSet mach) {
  machine_param_ = mach;
  // NOTE: Currently, we only support Grbl machine, 
  //       thus, no need to re-create motion controller when MachineSet is changed
  //       However, when any other motion controller type is supported, we should handle it
  // TODO: 
  //  1. For single active machine design, 
  //         delete and create new motion controller, if already connected
  //  2. For multiple active machine design,
  //         ???
}

/**
 * @brief Create and set the gcode job as the next job
 * 
 * @param gcode_list 
 * @return true 
 * @return false: cancelled, no job executor exists or already running a job
 */
bool Machine::createGCodeJob(QStringList gcode_list, QPointer<QProgressDialog> progress_dialog) {
  QList<Timestamp> timestamp_list;
  try {
    QElapsedTimer timer;
    qInfo() << "Start calcRequiredTime";
    timer.start();
    timestamp_list = MachineJob::calcRequiredTime(gcode_list, progress_dialog);
    qDebug() << "The calcRequiredTime took" << timer.elapsed() << "milliseconds";
  } catch (...) {
    // Terminated (Cancelled)
    return false;
  }
  auto job = QSharedPointer<GCodeJob>::create(gcode_list);
  job->setMotionController(motion_controller_);
  job->setTimestampList(timestamp_list);
  if (!job_executor_) {
    return false;
  }
  if (!job_executor_->setNewJob(job)) {
    return false;
  }

  return true;
}

/**
 * @brief Create and set the gcode job as the next job
 * 
 * @param gcode_list 
 * @return true 
 * @return false: cancelled, no job executor exists or already running a job
 */
bool Machine::createGCodeJob(QStringList gcode_list, QPixmap preview, QPointer<QProgressDialog> progress_dialog) {
  QList<Timestamp> timestamp_list;
  try {
    QElapsedTimer timer;
    qInfo() << "Start calcRequiredTime";
    timer.start();
    timestamp_list = MachineJob::calcRequiredTime(gcode_list, progress_dialog);
    qDebug() << "The calcRequiredTime took" << timer.elapsed() << "milliseconds";
  } catch (...) {
    // Terminated (Cancelled)
    return false;
  }
  auto job = QSharedPointer<GCodeJob>::create(gcode_list, preview);
  job->setMotionController(motion_controller_);
  job->setTimestampList(timestamp_list);
  if (!job_executor_) {
    return false;
  }
  if (!job_executor_->setNewJob(job)) {
    return false;
  }

  return true;
}

/**
 * @brief 
 * 
 * @param gcode_list 
 * @return true 
 * @return false: no job executor exists or already running a job
 */
bool Machine::createFramingJob(QStringList gcode_list) {
  auto job = QSharedPointer<FramingJob>::create(gcode_list);
  job->setMotionController(motion_controller_);
  if (!job_executor_) {
    return false;
  }
  if (!job_executor_->setNewJob(job)) {
    return false;
  }

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
  console_executor_->attachMotionController(motion_controller_);

  machine_setup_executor_->start();
}

void Machine::motionPortActivated() {
  rt_status_executor_->start();
}

void Machine::motionPortDisonnected() {
  // ...
  qInfo() << "Machine::motionPortDisonnected()";
}

void Machine::startJob() {
  job_executor_->start();
}

void Machine::pauseJob() {
  // Send pause cmd to motion controller
  if (console_executor_ && job_executor_) {
    if (job_executor_->getState() == Executor::State::kRunning) {
      // TODO: Add support other motion controller than grbl
      console_executor_->appendCmd(
        GrblCmdFactory::createGrblCmd(GrblCmdFactory::CmdType::kCtrlPause, motion_controller_)
      );
    }
  }
}

void Machine::resumeJob() {
  // Send resume cmd to motion controller
  if (console_executor_ && job_executor_) {
    if (job_executor_->getState() == Executor::State::kPaused) {
      // TODO: Add support other motion controller than grbl
      console_executor_->appendCmd(
        GrblCmdFactory::createGrblCmd(GrblCmdFactory::CmdType::kCtrlResume, motion_controller_)
      );
    }
  }
}

void Machine::stopJob() {
  // Send stop cmd to motion controller
  if (console_executor_ && job_executor_) {
    if (job_executor_->getState() == Executor::State::kRunning || 
        job_executor_->getState() == Executor::State::kPaused) {
      // TODO: Add support other motion controller than grbl
      console_executor_->appendCmd(
        GrblCmdFactory::createGrblCmd(GrblCmdFactory::CmdType::kCtrlReset, motion_controller_)
      );
    }
  }
}
