#ifndef MACHINEJOB_H
#define MACHINEJOB_H

#include <QObject>
#include <QString>
#include <QPixmap>
#include <QPointer>
#include <QProgressDialog>
#include <memory>
#include "../operation_cmd/operation_cmd.h"
#include <common/timestamp.h>

class MachineJob : public QObject
{
  Q_OBJECT
public:
  explicit MachineJob(QString job_name = "Job");

  virtual std::shared_ptr<OperationCmd> getNextCmd();
  virtual bool end() const;
  virtual void reload();
  virtual float getProgressPercent() const;
  int getIndex() const { return next_gcode_idx_; } 
  QString getJobName() const { return job_name_; }

  virtual Timestamp getElapsedTime() const;
  virtual Timestamp getTotalRequiredTime() const;
  virtual Timestamp getRemainingTime() const;
  
  bool withPreview() const;
  QPixmap getPreview() const;
  void setMotionController(QPointer<MotionController>);

  static QList<Timestamp> calcRequiredTime(const QStringList &gcode_list,
                                          QPointer<QProgressDialog> progress_dialog);
  static QList<Timestamp> calcRequiredTime(QStringList &&gcode_list,
                                          QPointer<QProgressDialog> progress_dialog);

protected:
  QString job_name_;
  bool with_preview_ = false;
  QPixmap preview_;
  qsizetype next_gcode_idx_ = 0;
  QStringList gcode_list_;

  QPointer<MotionController> motion_controller_;
};

#endif // MACHINEJOB_H
