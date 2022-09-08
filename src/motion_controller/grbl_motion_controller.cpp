#include "grbl_motion_controller.h"

#include <string>
#include <memory>
#include <QDebug>

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
  if (port_->write(cmd_packet) < 0) {
    return false;
  }
  qInfo() << "SND: " << cmd_packet;
  emit MotionController::cmdSent(cmd_packet);
  cbuf_occupied_ += cmd_packet.size();
  cmd_size_buf_.push_back(cmd_packet.size());

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
  std::string cmd;
  try {
    switch(ctrl_cmd) {
      case MotionControllerCtrlCmd::kStatusReport:      // ?
        cmd = "?";
        break;
      case MotionControllerCtrlCmd::kPause:          // !
        cmd = "!";
        break;
      case MotionControllerCtrlCmd::kResume:        // ~
        cmd = "~";
        break;
      case MotionControllerCtrlCmd::kSoftReset:         // 0x18
        cmd = "0x18";
        break;
      default:
        break;
    }
    if (port_->write(cmd) < 0) {
      port_tx_mutex_.unlock();
      qInfo() << "port_->write failed";
      return false;
    }
    qInfo() << "SND>" << cmd.c_str();
    emit MotionController::cmdSent(QString::fromStdString(cmd));
  } catch(...) {
    port_tx_mutex_.unlock();
    return false;
  }
  port_tx_mutex_.unlock();
  return true;
}

/**
 * @brief Handle response from GRBL board
 * 
 * @param resp 
 */
void GrblMotionController::respReceived(QString resp) {
  // TODO: Handle the case that the resp isn't a complete line or contains multiple lines
  //       Manually split resp here

  // Handle single line of resp
  //    "ok", "error", 
  //    "<....>", 
  //    "[MSG:...]", "[DEBUG:...]", "[FLUX:...]", ...
  qInfo() << "RECV<" << resp;
  resp = resp.trimmed();
  if (resp.contains(QString{"ok"}) || resp.contains(QString{"error"})) {
    if (resp.contains("error")) {
      //error_count += 1;
    }
    if (!cmd_size_buf_.isEmpty()){
      cmd_size_buf_.pop_front(); //  Delete the block character count corresponding to the last 'ok'
    }
    cbuf_occupied_ = std::accumulate(cmd_size_buf_.begin(), cmd_size_buf_.end(), 0);
    qInfo() << "cbuf_occupied: " << cbuf_occupied_;
    emit MotionController::ackRcvd();
  } else if (resp.startsWith("<")) {
    emit MotionController::realTimeStatusReceived();
    QRegularExpressionMatch match = rt_status_expr.match(resp);
    if (match.hasMatch()) {
      MotionControllerState new_state;
      if (match.captured("state").startsWith("Run") || 
          match.captured("state").startsWith("Jog") ||
          match.captured("state").startsWith("Home")) {
        new_state = MotionControllerState::kRun;
      } else if (match.captured("state").startsWith("Idle")) {
        new_state = MotionControllerState::kIdle;
      } else if (match.captured("state").startsWith("Alarm")) {
        new_state = MotionControllerState::kAlarm;
      } else if (match.captured("state").startsWith("Hold") || 
          match.captured("state").startsWith("Door")) {
        new_state = MotionControllerState::kPaused;
      } else if (match.captured("state").startsWith("Check")) {
        new_state = MotionControllerState::kCheck;
      } else if (match.captured("state").startsWith("Sleep")) {
        new_state = MotionControllerState::kSleep;
      } else {
        new_state = MotionControllerState::kUnknown;
      }
      auto res = std::make_shared<bool>();
      x_pos_ = match.captured("x_pos").toFloat(res.get());
      if (*res != true) {
        x_pos_ = 0;
      }
      y_pos_ = match.captured("y_pos").toFloat(res.get());
      if (*res != true) {
        y_pos_ = 0;
      }
      z_pos_ = match.captured("z_pos").toFloat(res.get());
      if (*res != true) {
        z_pos_ = 0;
      }
      qInfo() << match.captured("state");
      qInfo() << match.captured("pos_type");
      qInfo() << "(" << x_pos_ << ", " << y_pos_ << ", " << z_pos_ << ")";
      setState(new_state);
    }
  } else if (resp.startsWith("Grbl")) {
    // A reset occurred
    cmd_size_buf_.clear();
    cbuf_occupied_ = 0;
    x_pos_ = y_pos_ = z_pos_ = 0;
    // TODO: Parse Grbl version info
  } else if (resp.startsWith("FLUX")) {
    // e.g. FLUX Lazervida:0.1.2 Ready!
    // TODO: Parse FLUX machine model and fw version
  } else if (resp.startsWith("[FLUX:")) {
    // TODO: Handle [FLUX: ...]
    // Handle FLUX's dedicated response
  }

}
