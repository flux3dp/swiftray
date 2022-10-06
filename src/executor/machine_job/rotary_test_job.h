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
  explicit RotaryTestJob(QRectF bbox, char rotary_axis, qreal feedrate, 
                          QString job_name = "Rotary Test Job");

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

#endif // ROTARY_TEST_JOB_H
