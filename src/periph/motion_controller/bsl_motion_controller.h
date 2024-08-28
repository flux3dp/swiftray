#ifndef BSLMOTIONCONTROLLER_H
#define BSLMOTIONCONTROLLER_H

#include "motion_controller.h"

#include <QStringList>
#include <QRegularExpression>
#include <mutex>
#include <thread>
#include "liblcs/lcsApi.h"
#include "liblcs/lcsExpr.h"

class BSLMotionController : public MotionController
{
public:
  BSLMotionController(QObject *parent = nullptr);
  void attachPortBSL();
  bool detachPort() override;
  QString type() override { return "BSL"; }
  CmdSendResult stop() override;
  CmdSendResult pause();
  CmdSendResult resume();
  CmdSendResult sendCmdPacket(QPointer<Executor> executor, QString cmd_packet) override;

public Q_SLOTS:
  void respReceived(QString resp) override;

private:
  QStringList pending_cmds_;
  std::mutex cmd_list_mutex_;
  bool is_running_laser_ = false; 
  bool is_threading = false;
  bool should_flush_ = false;
  int buffer_size_ = 0;
  enum class AlarmCode {
    kNone = 0,
    kMin = 1,
    kHardLimit = 1,
    kSoftLimit = 2,
    kAbortCycle = 3,
    kMax = 10,
  };
  double current_x = 0.0;
  double current_y = 0.0;
  double current_f = 6000.0; // Default speed
  std::thread command_runner_thread_;
  void handleGcode(const QString &cmd_packet);
  LCS2Error waitListAvailable(int list_no);
  void startCommandRunner();
  void commandRunnerThread();
  void dequeueCmd(int count);
  QString getAlarmMsg(AlarmCode code);
};

#endif // BSLMOTIONCONTROLLER_H
