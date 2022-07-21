#include <motion_controller_job/base-job.h>

BaseJob::BaseJob(QObject *parent, QString endpoint, QVariant data) :
     QThread(parent) {}

BaseJob::~BaseJob() {}

BaseJob::Status BaseJob::status() {
  return status_;
}

void BaseJob::setStatus(BaseJob::Status status) {
  status_ = status;
  emit statusChanged(status_);
}

int BaseJob::progress() { return 1; };

void BaseJob::start() {};

void BaseJob::stop() {};

void BaseJob::pause() {};

void BaseJob::resume() {};

QString BaseJob::statusToString(BaseJob::Status status) {
  switch (status) {
    case BaseJob::Status::READY:
      return tr("Ready");
    case BaseJob::Status::STARTING:
      return tr("Starting");
    case BaseJob::Status::RUNNING:
      return tr("Running");
    case BaseJob::Status::PAUSED:
      return tr("Paused");
    case BaseJob::Status::ALARM:
      return tr("Alarm");
    case BaseJob::Status::STOPPED:
      return tr("Stopped");
    case BaseJob::Status::ALARM_STOPPED:
      return tr("Alarm Stopped");
    case BaseJob::Status::FINISHED:
      return tr("Finished");
    default:
      return tr("Undefined Status");
  }
}

void BaseJob::setTimestampList(QList<QTime> &&timestamp_list) {
  timestamp_list_ = timestamp_list;
}

void BaseJob::setTimestampList(const QList<QTime> &timestamp_list) {
  timestamp_list_ = timestamp_list;
}

QTime BaseJob::getTimestamp(int idx) {
  if (timestamp_list_.empty()) {
    return QTime{0, 0};
  } else if (idx >= timestamp_list_.count()) {
    return timestamp_list_.last();
  }
  return timestamp_list_.at(idx);
}

QTime BaseJob::getTotalRequiredTime() {
  if (timestamp_list_.empty()) {
    return QTime{0, 0};
  }
  return timestamp_list_.last();
}

QTime BaseJob::getElapsedTime() {
  if (timestamp_list_.count() <= getFinishedCmdIdx()) {
    return QTime{0, 0};
  }
  return timestamp_list_.at(getFinishedCmdIdx());
}
