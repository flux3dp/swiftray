#ifndef CONSOLE_JOB_H
#define CONSOLE_JOB_H

#include "machine_job.h"
#include <periph/motion_controller/motion_controller.h>
#include <QPointer>
#include <QStringList>
#include <common/timestamp.h>

class ConsoleJob : public MachineJob
{
  Q_OBJECT
public:
  explicit ConsoleJob(QString command, QString job_name = "console Job");

  void setMotionController(QPointer<MotionController>);

  std::shared_ptr<OperationCmd> getNextCmd() override;
  bool end() const override;
  void reload() override;
  float getProgressPercent() const override;
  Timestamp getElapsedTime() const override;
  Timestamp getTotalRequiredTime() const override;
  Timestamp getRemainingTime() const override;

private:
  qsizetype next_gcode_idx_ = 0;
  QStringList gcode_list_;

  QPointer<MotionController> motion_controller_;

};

#endif // CONSOLE_JOB_H
