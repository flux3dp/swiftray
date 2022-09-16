#ifndef GCODEJOB_H
#define GCODEJOB_H

#include "machine_job.h"
#include <periph/motion_controller/motion_controller.h>
#include <QStringList>
#include <QPointer>

class GCodeJob : public MachineJob
{
public:
  explicit GCodeJob(QStringList gcode_list, QString job_name = "GCodeJob");
  explicit GCodeJob(QString gcodes, QString job_name = "GCodeJob");

  void setMotionController(QPointer<MotionController>);

  bool isActive() override;
  std::shared_ptr<OperationCmd> getNextCmd() override;
  bool end() override;
  void reload() override;

private:
  qsizetype current_gcode_idx_ = 0;
  QStringList gcode_list_;

  QPointer<MotionController> motion_controller_;
};

#endif // GCODEJOB_H
