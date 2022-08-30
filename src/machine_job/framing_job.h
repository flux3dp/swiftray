#ifndef FRAMINGJOB_H
#define FRAMINGJOB_H

#include "machine_job.h"

class FramingJob : public MachineJob
{
public:
  explicit FramingJob(QObject *parent = nullptr);
};

#endif // FRAMINGJOB_H
