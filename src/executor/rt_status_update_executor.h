#ifndef RTSTATUSUPDATEEXECUTOR_H
#define RTSTATUSUPDATEEXECUTOR_H

#include <QObject>
#include "executor.h"
#include <motion_controller/motion_controller.h>
#include <QPointer>

class RTStatusUpdateExecutor : public Executor
{
  Q_OBJECT
public:
  explicit RTStatusUpdateExecutor(QPointer<MotionController> motion_controller, 
                                  QObject *parent = nullptr);

public slots:
  void exec() override;

signals:
  void hanging();

private:
  QPointer<MotionController> motion_controller_;
};

#endif // RTSTATUSUPDATEEXECUTOR_H
