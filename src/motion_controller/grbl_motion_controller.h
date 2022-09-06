#ifndef GRBLMOTIONCONTROLLER_H
#define GRBLMOTIONCONTROLLER_H

#include "motion_controller.h"

#include <QStringList>
#include <mutex>

class GrblMotionController : public MotionController
{
public:
  explicit GrblMotionController(QObject *parent = nullptr);

  bool sendCmdPacket(QString cmd_packet) override;
  bool sendSysCmd(MotionControllerSystemCmd sys_cmd) override;
  bool sendCtrlCmd(MotionControllerCtrlCmd ctrl_cmd) override;

public slots:
  void respReceived(QString resp);

private:
  std::mutex port_tx_mutex_;
  size_t cbuf_space_ = 80;    // TODO: Assign settings to it
  QStringList cmd_in_buf_;
  size_t cbuf_occupied_ = 0;  // sum of size of all cmds in buffer
};

#endif // GRBLMOTIONCONTROLLER_H
