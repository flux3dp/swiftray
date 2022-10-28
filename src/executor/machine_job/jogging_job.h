#ifndef JOGGING_JOB_H
#define JOGGING_JOB_H

#include "machine_job.h"
#include <periph/motion_controller/motion_controller.h>
#include <QPointer>
#include <QStringList>
#include <common/timestamp.h>

class JoggingRelativeJob : public MachineJob
{
  Q_OBJECT
public:
  explicit JoggingRelativeJob(qreal x_pos, qreal y_pos, 
                      qreal feedrate, QString job_name = "JoggingRelativeJob");

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

class JoggingAbsoluteJob : public MachineJob
{
  Q_OBJECT
public:
  explicit JoggingAbsoluteJob(bool x_exist, qreal x_pos, bool y_exist, qreal y_pos, 
                            qreal feedrate, QString job_name = "JoggingAbsoluteJob");

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

#endif // JOGGING_JOB_H
