#ifndef ROTARY_TEST_JOB_H
#define ROTARY_TEST_JOB_H

#include "machine_job.h"
#include <periph/motion_controller/motion_controller.h>
#include <QPointer>
#include <QStringList>
#include <common/timestamp.h>

class RotaryTestJob : public MachineJob
{
  Q_OBJECT
public:
  explicit RotaryTestJob(QRectF bbox, char rotary_axis, qreal feedrate, double framing_power,
                          QString job_name = "Rotary Test Job");
};

#endif // ROTARY_TEST_JOB_H
