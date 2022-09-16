#ifndef MACHINEJOB_H
#define MACHINEJOB_H

#include <QString>
#include <memory>
#include "../operation_cmd/operation_cmd.h"

class MachineJob
{
public:
  explicit MachineJob(QString job_name = "Job");

  virtual bool isActive() = 0;
  virtual std::shared_ptr<OperationCmd> getNextCmd() = 0;
  virtual bool end() = 0;
  virtual void reload() = 0;

private:
  QString job_name_;

};

#endif // MACHINEJOB_H
