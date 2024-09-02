#ifndef RTSTATUSUPDATEEXECUTOR_H
#define RTSTATUSUPDATEEXECUTOR_H

#include <QObject>
#include "executor.h"
#include <QPointer>
#include <QTimer>

class RTStatusUpdateExecutor : public Executor
{
  Q_OBJECT
public:
  explicit RTStatusUpdateExecutor(QObject *parent = nullptr);
  void handleCmdFinish(int) override {};


Q_SIGNALS:
  void timeout();
  void startWatchdog();
  void stopWatchdog();

private Q_SLOTS:
  void exec() override;
  void handleStopped() override;
  void handleMotionControllerStatusUpdate(MotionControllerState mc_state, qreal x_pos, qreal y_pos, qreal z_pos) override;
  void onStartWatchdog();
  void onStopWatchdog();

private:
  QTimer *watchdog_timer_;
};

#endif // RTSTATUSUPDATEEXECUTOR_H
