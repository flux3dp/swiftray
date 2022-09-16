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
  void handleCmdFinish(int) override;

  void attachMotionController(QPointer<MotionController> motion_controller);

public slots:
  void start() override;
  void exec() override;
  void pause() override;
  void resume() override;
  void stop() override;

  void onReportRcvd();

signals:
  void hanging();

private:

  QPointer<MotionController> motion_controller_;
  bool hanging_ = false;
  QTimer *exec_timer_;
  QTimer *hangning_detect_timer_;
};

#endif // RTSTATUSUPDATEEXECUTOR_H
