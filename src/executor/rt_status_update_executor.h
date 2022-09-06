#ifndef RTSTATUSUPDATEEXECUTOR_H
#define RTSTATUSUPDATEEXECUTOR_H

#include <QObject>
#include "executor.h"
#include <motion_controller/motion_controller.h>
#include <QPointer>
#include <QTimer>

class RTStatusUpdateExecutor : public Executor
{
  Q_OBJECT
public:
  explicit RTStatusUpdateExecutor(QPointer<MotionController> motion_controller, 
                                  QObject *parent = nullptr);
  ~RTStatusUpdateExecutor();

public slots:
  void start() override;
  void exec() override;
  void stop() override;

  void onReportRcvd();

signals:
  void hanging();

private:
  QPointer<MotionController> motion_controller_;
  bool responded_ = true;
  QTimer *timer_;
  QTimer *hangning_detect_timer_;
};

#endif // RTSTATUSUPDATEEXECUTOR_H
