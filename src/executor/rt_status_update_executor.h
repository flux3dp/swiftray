#ifndef RTSTATUSUPDATEEXECUTOR_H
#define RTSTATUSUPDATEEXECUTOR_H

#include <QObject>
#include "executor.h"
#include <motion_controller/motion_controller.h>

class RTStatusUpdateExecutor : public Executor
{
  Q_OBJECT
public:
  explicit RTStatusUpdateExecutor(MotionController *motion_controller, 
                                  QObject *parent = nullptr);

  void start() override;

signals:
  void hanging();

private:
  MotionController *motion_controller_;
};

#endif // RTSTATUSUPDATEEXECUTOR_H
