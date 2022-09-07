#include "grbl_motion_controller.h"

#include <string>
#include <QDebug>
#include <regex>

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
      case MotionControllerCtrlCmd::kFeedHold:          // !
        cmd = "!";
        break;
      case MotionControllerCtrlCmd::kCycleStart:        // ~
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
    // Only match some of the necessary info, reduce the workload
    std::cmatch m;
    std::regex rt_status_regex(
      "<"
      "(Idle|Run|Jog|Home|Alarm|Check|Sleep|Hold:\\d|Door:\\d)"
      "(\\|(MPos:|WPos:)([+-]?([0-9]*[.])?[0-9]+),([+-]?([0-9]*[.])?[0-9]+),([+-]?([0-9]*[.])?[0-9]+))"
      ".*"
      ">");
    std::regex_match(resp, m, rt_status_regex);
    for (unsigned i=0; i < m.size(); ++i) {
      qInfo() << "match " << i << ": " << m[i];
    }
    //qInfo() << "rt status: " << resp;
    //auto len = resp.indexOf('>') - 1;
    //auto extracted = out_temp.mid(1, len > 0 ? len : 0);
    //auto status_tokens = extracted.split('|', Qt::SkipEmptyParts);
    //handleRealtimeStatusReport(status_tokens);
    //if (status() == Status::ALARM) {
    //  throw "ALARM";
    //}
  }
}
