#ifndef FRAMINGJOB_H
#define FRAMINGJOB_H

#include "machine_job.h"

class FramingJob : public MachineJob
{
public:
  explicit FramingJob(QString job_name = "FramingJob");

  bool isActive() override;
  std::shared_ptr<OperationCmd> getNextCmd() override;
  bool end() override;
  void reload() override;
};

#endif // FRAMINGJOB_H
