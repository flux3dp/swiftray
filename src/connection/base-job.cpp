#include <connection/base-job.h>

BaseJob::BaseJob(QObject *parent, QString endpoint, QVariant data) : QThread(parent) {

}
