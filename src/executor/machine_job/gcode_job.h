#ifndef GCODEJOB_H
#define GCODEJOB_H

#include "machine_job.h"
#include <periph/motion_controller/motion_controller.h>
#include <QStringList>
#include <QPointer>
#include <QPixmap>
#include <common/timestamp.h>

class GCodeJob : public MachineJob
{
  Q_OBJECT
public:
  explicit GCodeJob(QStringList gcode_list, QString job_name = "GCodeJob");
  explicit GCodeJob(QString gcodes, QString job_name = "GCodeJob");
  explicit GCodeJob(QStringList gcode_list, QPixmap preview, QString job_name = "GCodeJob");
  explicit GCodeJob(QString gcodes, QPixmap preview, QString job_name = "GCodeJob");

  void setMotionController(QPointer<MotionController>);
  void setTimestampList(const QList<Timestamp> &ts_list);
  void setTimestampList(QList<Timestamp> &&ts_list);

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
  QList<Timestamp> timestamp_list_;

  QPointer<MotionController> motion_controller_;
};

#endif // GCODEJOB_H
