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
 *        
 * @param executor cmd sender (cmd source)
 * @param cmd_packet 
 * @return true if cmd_packet is sent
 * @return false if port is busy or buffer is full
 */
MotionController::CmdSendResult GrblMotionController::sendCmdPacket(QPointer<Executor> executor, QString cmd_packet) {
  size_t newline_cnt = cmd_packet.count(QChar('\n'));
  if (newline_cnt > 1) {
    // TODO: return other value to tell apart from buffer full?
    return CmdSendResult::kInvalid;
  }

  // Special handling for control command
  if (newline_cnt == 0) { // No "ok" or "error" expected
  //if (cmd_packet.size() == 1 && 
  //    (cmd_packet.at(0) == '?' || cmd_packet.at(0) == '!' || cmd_packet.at(0) == '~' || cmd_packet.at(0) > 0x7f )) {
    // NOTE: Ctrl cmd is handled immediately so it won't occupy cmd buffer.
    //       Thus, no need to consider cmd_in_buf
    port_tx_mutex_.lock();
    try {
      #ifdef CUSTOM_SERIAL_PORT_LIB
      int result = port_->write(cmd_packet);
      if (result < 0) {
      #else
      int result = port_->write(cmd_packet.toStdString().c_str());
      if (result < 0) {
      #endif
        port_tx_mutex_.unlock();
        qInfo() << "port_->write failed";
        return CmdSendResult::kFail;
      }
      qInfo() << "SND>" << cmd_packet;
      emit MotionController::cmdSent(cmd_packet);
    } catch(...) {
      port_tx_mutex_.unlock();
      return CmdSendResult::kFail;
    }
    port_tx_mutex_.unlock();
    return CmdSendResult::kOk;
  }

  // Normal handling line of command, consider the available buffer space at first
  // newline_cnt == 1, expect an "ok" or "error" resp
  std::unique_lock<std::mutex> lk(port_tx_mutex_, std::try_to_lock);
  if (!lk.owns_lock()) {
      // mutex wasn't locked. Handle it.
      return CmdSendResult::kBusy;
  }

  if (cbuf_space_ < cbuf_occupied_ + cmd_packet.size()) {
    return CmdSendResult::kBusy;
  }
  #ifdef CUSTOM_SERIAL_PORT_LIB
  int result = port_->write(cmd_packet);
  if (result < 0) {
  #else
  int result = port_->write(cmd_packet.toStdString().c_str());
  if (result < 0) {
  #endif
    return CmdSendResult::kFail;
  }
  qInfo() << "SND: " << cmd_packet;
  cbuf_occupied_ += cmd_packet.size();
  cmd_size_buf_.push_back(cmd_packet.size());
  enqueueCmdExecutor(executor);
  emit MotionController::cmdSent(cmd_packet);

  return CmdSendResult::kOk;
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
  emit MotionController::respRcvd(resp);
  
  resp = resp.trimmed();
  if (resp.contains(QString{"ok"}) || resp.contains(QString{"error"})) {
    int result_code = 0; 
    if (resp.contains("error")) {
      result_code = 1;// TODO: To be parsed from resp
      //error_count += 1;
    }
    if (!cmd_size_buf_.isEmpty()){
      cmd_size_buf_.pop_front(); //  Delete the block character count corresponding to the last 'ok'
    }
    cbuf_occupied_ = std::accumulate(cmd_size_buf_.begin(), cmd_size_buf_.end(), 0);
    if (!cmd_executor_queue_.isEmpty()) {
      if (!cmd_executor_queue_.at(0).isNull()) {
        cmd_executor_queue_.at(0)->handleCmdFinish(result_code);
      }
      dequeueCmdExecutor();
    }
    qInfo() << "cbuf_occupied: " << cbuf_occupied_;
  } else if (resp.startsWith("<")) {
    MotionControllerState old_state = state_;
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
      //qInfo() << match.captured("pos_type");
      //qInfo() << "(" << x_pos_ << ", " << y_pos_ << ", " << z_pos_ << ")";
      setState(new_state);
      emit MotionController::realTimeStatusUpdated(old_state, state_, x_pos_, y_pos_, z_pos_);
    }
  } else if (resp.startsWith("Grbl")) {
    // A reset occurred
    cmd_executor_queue_.clear();
    cmd_size_buf_.clear();
    cbuf_occupied_ = 0;
    x_pos_ = y_pos_ = z_pos_ = 0;

    emit MotionController::resetDetected();
    // TODO: Parse Grbl version info
  } else if (resp.startsWith("[MSG:")) {
    // TODO: Handle Grbl msg immediately
    if (resp.contains("Reset to continue")) {
      sendCmdPacket(nullptr, "\x18");
    } else if (resp.contains("'$H'|'$X' to unlock")) {
      sendCmdPacket(nullptr, "$X\n");
    } else if (resp.contains("Pgm End")) {
      // TODO:
    } else if (resp.contains("Restoring defaults")) {
      // TODO:
    } else if (resp.contains("Sleeping")) {
      // TODO:
    } else if (resp.contains("Check Door")) {
      emit MotionController::notif(tr("NOTICE"), tr("Please check machine door."));
    } else if (resp.contains("Check Bottom")) {
      emit MotionController::notif(tr("NOTICE"), tr("Please check machine bottom."));
    }
  } else if (resp.startsWith("ALARM:")) {
    MotionControllerState old_state = state_;
    qInfo() << "Alarm detected";
    QString subString = resp.mid(strlen("ALARM:"));
    int code = subString.toInt();
    if (code >= static_cast<int>(AlarmCode::kMin) &&
        code <= static_cast<int>(AlarmCode::kMax) && 
        code != static_cast<int>(AlarmCode::kAbortCycle) &&
        code != static_cast<int>(AlarmCode::kHomingFailReset)) {
      // NOTE: No need to show message dialog when abort during cycle
      emit MotionController::notif(tr("Alarm: ") + QString::number(code), getAlarmMsg(static_cast<AlarmCode>(code)));
    } else {
      // Unknown alarm code
    }
    setState(MotionControllerState::kAlarm);
    emit MotionController::realTimeStatusUpdated(old_state, state_, x_pos_, y_pos_, z_pos_);
  } else if (resp.startsWith("[VER:]")) {
    // TODO: Parse grbl version
  } else if (resp.startsWith("[OPT:]")) {
    // TODO: Parse block buffer size & serial buffer size
    //       * update cbuf_space_ based on serial buffer size
    //       * update xxx based on block buffer size
  } else if (resp.startsWith("FLUX")) {
    // e.g. FLUX Lazervida:0.1.2 Ready!
    // TODO: Parse FLUX machine model and fw version
  } else if (resp.startsWith("[FLUX:")) {
    // TODO: Handle [FLUX: ...]
    // Handle FLUX's dedicated responses:
    if (resp.contains("act")) {
      emit MotionController::notif(tr("NOTICE"), tr("Machine is paused by drop or collision."));
    } else if (resp.contains("tilt")) {
      emit MotionController::notif(tr("NOTICE"), tr("Machine is paused by tilt."));
    }
    // * [FLUX: act]
    // * [FLUX: tilt]
  } 

}

QString GrblMotionController::getAlarmMsg(AlarmCode code) {
  switch (code) {
    case AlarmCode::kHardLimit:
      return tr("Hard limit");
    case AlarmCode::kSoftLimit:
      return tr("Soft limit");
    case AlarmCode::kAbortCycle:
      return tr("Abort during cycle");
    case AlarmCode::kProbeFailInitial:
      return tr("Probe fail");
    case AlarmCode::kProbeFailContact:
      return tr("Probe fail");
    case AlarmCode::kHomingFailReset:
      return tr("Homing fail");
    case AlarmCode::kHomingFailDoor:
      return tr("Homing fail");
    case AlarmCode::kHomingFailPullOff:
      return tr("Homing fail");
    case AlarmCode::kHomingFailApproach:
      return tr("Homing fail");
    case AlarmCode::kHomingFailDualApproach:
      return tr("Homing fail");
    default:
      return "Unknown alarm";
  }
}
