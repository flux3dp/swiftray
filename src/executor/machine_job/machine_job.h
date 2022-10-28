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

  virtual std::shared_ptr<OperationCmd> getNextCmd() = 0;
  virtual bool end() const = 0;
  virtual void reload() = 0;
  virtual float getProgressPercent() const = 0;
  virtual Timestamp getElapsedTime() const = 0;
  virtual Timestamp getTotalRequiredTime() const = 0;
  virtual Timestamp getRemainingTime() const = 0;
  
  bool withPreview() const;
  QPixmap getPreview() const;

  static QList<Timestamp> calcRequiredTime(const QStringList &gcode_list,
                                          QPointer<QProgressDialog> progress_dialog);
  static QList<Timestamp> calcRequiredTime(QStringList &&gcode_list,
                                          QPointer<QProgressDialog> progress_dialog);

protected:
  QString job_name_;
  bool with_preview_ = false;
  QPixmap preview_;
};

#endif // MACHINEJOB_H
