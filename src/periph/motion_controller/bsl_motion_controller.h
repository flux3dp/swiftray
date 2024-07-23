#ifndef BSLMOTIONCONTROLLER_H
#define BSLMOTIONCONTROLLER_H

#include "motion_controller.h"

#include <QStringList>
#include <QRegularExpression>
#include <mutex>

class BSLMotionController : public MotionController
{
public:
  explicit BSLMotionController(QObject *parent = nullptr);
  void attachPortBSL();
  CmdSendResult stop() override;
  CmdSendResult pause();
  CmdSendResult resume();
  CmdSendResult sendCmdPacket(QPointer<Executor> executor, QString cmd_packet) override;

public Q_SLOTS:
  void respReceived(QString resp) override;

private:
  void handleGcode(QString cmd_packet);
  std::mutex port_tx_mutex_;
  bool is_running_laser_ = false; 
  enum class AlarmCode {
    kNone = 0,
    kMin = 1,
    kHardLimit = 1,
    kSoftLimit = 2,
    kAbortCycle = 3,
    kMax = 10,
  };

  QString getAlarmMsg(AlarmCode code);
};

#endif // BSLMOTIONCONTROLLER_H
