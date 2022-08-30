#ifndef GCODEJOB_H
#define GCODEJOB_H

#include <machine_job.h>
#include <QStringList>

class GCodeJob : public MachineJob
{
public:
  explicit GCodeJob(QStringList gcode_list, QString job_name = "GCodeJob", QObject *parent = nullptr);
  explicit GCodeJob(QString gcodes, QString job_name = "GCodeJob", QObject *parent = nullptr);

  bool isActive() override;
  std::tuple<Target, QString> getNextCmd() override;
  bool end() override;

private:
  qsizetype current_gcode_idx_ = 0;
  QStringList gcode_list_;
};

#endif // GCODEJOB_H
