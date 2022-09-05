#include "grbl_motion_controller.h"

#include <string>

GrblMotionController::GrblMotionController(QObject *parent)
  : MotionController{parent}
{

}

/**
 * @brief Send the cmd_packet immediately
 *        But consider the available buffer space at first
 * @param cmd_packet 
 * @return true if cmd_packet is sent
 * @return false if port is busy or buffer is full
 */
bool GrblMotionController::sendCmdPacket(QString cmd_packet) {
  std::unique_lock<std::mutex> lk(port_tx_mutex_, std::try_to_lock);
  if (!lk.owns_lock()) {
      // mutex wasn't locked. Handle it.
      return false;
  }

  if (cbuf_space_ < cbuf_occupied_ + cmd_packet.size()) {
    return false;
  }
  port_->write(cmd_packet);
  cmd_in_buf_.push_back(cmd_packet);
  cbuf_occupied_ += cmd_packet.size();

  return true;
}

bool GrblMotionController::sendSysCmd(MotionControllerSystemCmd sys_cmd) {
  // NOTE: port lock is handled in sendCmdPacket
  switch(sys_cmd) {
    case MotionControllerSystemCmd::kViewParserState:   // $G
      return sendCmdPacket("$G\n");
    case MotionControllerSystemCmd::kUnlock:            // $X
      return sendCmdPacket("$X\n");
    case MotionControllerSystemCmd::kHome:              // $H
      return sendCmdPacket("$H\n");
    case MotionControllerSystemCmd::kViewGrblSettings:  // $$
      return sendCmdPacket("$$\n");
    case MotionControllerSystemCmd::kViewBuildInfo:     // $I
      return sendCmdPacket("$I\n");
    case MotionControllerSystemCmd::kViewStartupBlocks: // $N
      return sendCmdPacket("$N\n");
    case MotionControllerSystemCmd::kToggleCheckMode:   // $C
      return sendCmdPacket("$C\n");
    default:
      // Unsupported cmd, consider as sent?
      return true;
  }
}

bool GrblMotionController::sendCtrlCmd(MotionControllerCtrlCmd ctrl_cmd) {
  // NOTE: Ctrl cmd is handled immediately so it won't occupy cmd buffer.
  //       Thus, no need to consider cmd_in_buf
  port_tx_mutex_.lock();
  try {
    switch(ctrl_cmd) {
      case MotionControllerCtrlCmd::kStatusReport:      // ?
        port_->write(static_cast<std::string>("?"));
        break;
      case MotionControllerCtrlCmd::kFeedHold:          // !
        port_->write(static_cast<std::string>("!"));
        break;
      case MotionControllerCtrlCmd::kCycleStart:        // ~
        port_->write(static_cast<std::string>("~"));
        break;
      case MotionControllerCtrlCmd::kSoftReset:         // 0x18
        port_->write(static_cast<std::string>("\x18"));
        break;
      default:
        break;
    }
  } catch(...) {
    port_tx_mutex_.unlock();
    return false;
  }
  port_tx_mutex_.unlock();
  return true;
}
