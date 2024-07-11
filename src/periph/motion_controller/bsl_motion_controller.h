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
  CmdSendResult sendCmdPacket(QPointer<Executor> executor, QString cmd_packet) override;

public Q_SLOTS:
  void respReceived(QString resp) override;

private:
  void handleGcode(QString cmd_packet);
  std::mutex port_tx_mutex_;
  size_t cbuf_space_ = 80;    // TODO: Assign settings to it
  QList<size_t> cmd_size_buf_;
  size_t cbuf_occupied_ = 0;  // sum of size of all cmds in buffer
  
  // Only match some of the necessary info, reduce the workload
  QRegularExpression rt_status_expr{
      "<"
      "(?<state>Idle|Run|Jog|Home|Alarm|Check|Sleep|Hold:\\d|Door:\\d)"
      "("
        "\\|(?<pos_type>MPos:|WPos:)"
        "(?<x_pos>[+-]?([0-9]*[.])?[0-9]+)"
        "(,(?<y_pos>[+-]?([0-9]*[.])?[0-9]+))?"
        "(,(?<z_pos>[+-]?([0-9]*[.])?[0-9]+))?"
      ")"
      ".*"
      ">"
   };

  enum class AlarmCode {
    kNone = 0,
    kMin = 1,

    kHardLimit = 1,
    kSoftLimit = 2,
    kAbortCycle = 3,
    kProbeFailInitial = 4,
    kProbeFailContact = 5,
    kHomingFailReset = 6,
    kHomingFailDoor = 7,
    kHomingFailPullOff = 8,
    kHomingFailApproach = 9,
    kHomingFailDualApproach = 10,

    kMax = 10,
  };

  QString getAlarmMsg(AlarmCode code);

};

#endif // BSLMOTIONCONTROLLER_H
