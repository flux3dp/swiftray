#ifndef GRBLMOTIONCONTROLLER_H
#define GRBLMOTIONCONTROLLER_H

#include "motion_controller.h"

#include <QStringList>
#include <QRegularExpression>
#include <mutex>

class GrblMotionController : public MotionController
{
public:
  explicit GrblMotionController(QObject *parent = nullptr);

  bool sendCmdPacket(QString cmd_packet) override;
  bool sendSysCmd(MotionControllerSystemCmd sys_cmd) override;
  bool sendCtrlCmd(MotionControllerCtrlCmd ctrl_cmd) override;

public slots:
  void respReceived(QString resp) override;

private:
  std::mutex port_tx_mutex_;
  size_t cbuf_space_ = 80;    // TODO: Assign settings to it
  QList<size_t> cmd_size_buf_;
  size_t cbuf_occupied_ = 0;  // sum of size of all cmds in buffer
  
  // Grbl Status info
  qreal x_pos_ = 0;
  qreal y_pos_ = 0;
  qreal z_pos_ = 0;

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
};

#endif // GRBLMOTIONCONTROLLER_H
