#include <connection/base-job.h>

BaseJob::BaseJob(QObject *parent, QString endpoint, QVariant data) :
     QThread(parent) {}

BaseJob::~BaseJob() {}

BaseJob::Status BaseJob::status() {
  return status_;
}

void BaseJob::setStatus(BaseJob::Status status) {
  status_ = status;
}

float BaseJob::progress() { return 1.0; };

void BaseJob::start() {};

void BaseJob::pause() {};

void BaseJob::resume() {};