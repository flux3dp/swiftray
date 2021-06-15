#ifndef MEMORY_MONITOR_H
#define MEMORY_MONITOR_H

#include <QObject>
#include <mach/mach.h> //memory only

class MemoryMonitor
     : public QObject {
public:
  QString system_info_;
public slots:

  void doWork() {
    struct task_basic_info t_info;
    mach_msg_type_number_t t_info_count = TASK_BASIC_INFO_COUNT;

    if (KERN_SUCCESS == task_info(mach_task_self(),
                                  TASK_BASIC_INFO, (task_info_t) &t_info,
                                  &t_info_count)) {
      system_info_ = "";
      QTextStream out(&system_info_);
      out << "Resident " << (t_info.resident_size / (1024 * 1024)) << "mb virtual "
          << (t_info.virtual_size / (1024 * 1024)) << "mb";
    }
    if (QThread::currentThread()->isInterruptionRequested()) {
      qDebug() << "MemoryMonitor Stopped";
      QThread::currentThread()->exit(0);
      return;
    }
    QThread::sleep(1);
    doWork();
  };
};

#endif