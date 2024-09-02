#ifndef FRAMINGJOB_H
#define FRAMINGJOB_H

#include "machine_job.h"
#include <periph/motion_controller/motion_controller.h>
#include <QPointer>
#include <QStringList>
#include <common/timestamp.h>

class FramingJob : public MachineJob
{
  Q_OBJECT
public:
  explicit FramingJob(QStringList gcode_list, QString job_name = "FramingJob");
  explicit FramingJob(QString gcodes, QString job_name = "FramingJob");
};

#endif // FRAMINGJOB_H
