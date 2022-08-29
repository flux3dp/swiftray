#include "machine-controller.h"
#include <QThread>

MachineController::MachineController(const SerialPort *port,
                                     QSharedPointer<MotionController> motion_ctrler,
                                     QObject *parent)
  : port_{port}, motion_controller_{motion_ctrler}, QObject{parent}
{

  QThread *rt_status_thread = new QThread;
  MachineRTStatusUpdater *rt_status_updater = new MachineRTStatusUpdater();
  rt_status_updater->moveToThread(rt_status_thread);
  connect(rt_status_updater, &MachineRTStatusUpdater::error, this, &MachineController::errorString);
  connect(rt_status_thread, &QThread::started, rt_status_updater, &MachineRTStatusUpdater::process);
  connect(rt_status_updater, &MachineRTStatusUpdater::finished, rt_status_thread, &QThread::quit);
  connect(rt_status_updater, &MachineRTStatusUpdater::finished, rt_status_updater, &MachineRTStatusUpdater::deleteLater);
  connect(rt_status_thread, &QThread::finished, rt_status_thread, &QThread::deleteLater);
  rt_status_thread->start();


}

bool MachineController::setNewJob(QSharedPointer<MachineJob> new_job) {
  if (!job_mutex_.tryLock()) {
    return false;
  }
  if (current_job_) {
    if (current_job_->isActive()) {
      return false;
    }
    current_job_.reset();
  }
  current_job_ = new_job;

  job_mutex_.unlock();
  return true;
}

void MachineController::send(QString raw_msg) {
  // 1. Create cmd packet from raw_msg (by on motion_controller)
  // TODO:
  QString packet{raw_msg};
  // 2.
  port_->write(packet);
}

