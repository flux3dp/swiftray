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

#endif // FRAMINGJOB_H
