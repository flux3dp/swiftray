#include <connection/base-job.h>

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
