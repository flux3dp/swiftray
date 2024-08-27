#ifndef RTSTATUSUPDATEEXECUTOR_H
#define RTSTATUSUPDATEEXECUTOR_H

#include <QObject>
#include "executor.h"
#include <periph/motion_controller/motion_controller.h>
#include <QPointer>
#include <QTimer>

class RTStatusUpdateExecutor : public Executor
{
  Q_OBJECT
public:
  explicit RTStatusUpdateExecutor(QObject *parent = nullptr);
  void attachMotionController(QPointer<MotionController> motion_controller);
  void start() override;
  void handleCmdFinish(int) override;


Q_SIGNALS:
  void hanging();

private Q_SLOTS:
  void exec() override;
  void handlePaused() override;
  void handleResume() override;
  void handleStopped() override;
  void onReportRcvd();

private:

  QPointer<MotionController> motion_controller_;
  bool hanging_ = false;
  QTimer *exec_timer_;
  QTimer *hangning_detect_timer_;
};

#endif // RTSTATUSUPDATEEXECUTOR_H
