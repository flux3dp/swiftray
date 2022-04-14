#include <motion_controller_job/grbl-job.h>
#include <QIODevice>
#include <QMessageBox>
#include <QDebug>
#include <QSerialPortInfo>
#include <QTimer>
#include <QRegularExpression>
#include <numeric>

#include <connection/serial-port.h>
#include <motion_controller_job/grbl-job.h>

GrblJob::GrblJob(QObject *parent, QString endpoint, const QVariant &gcode) :
     BaseJob(parent, endpoint, gcode) {
  gcode_ = gcode.toStringList();

  //connect(&(SerialPort::getInstance()), &SerialPort::responseReceived, this, &GrblJob::parseResponse);
  connect(&(SerialPort::getInstance()), &SerialPort::responseReceived, this, &GrblJob::onResponseReceived);
  connect(&(SerialPort::getInstance()), &SerialPort::disconnected, this, &GrblJob::onPortDisconnected);

  real_time_status_timer_ = new QTimer(this);
  connect(this, &GrblJob::startRealTimeStatusTimer, this, &GrblJob::onStartRealTimeStatusTimer);
  connect(this, &GrblJob::stopRealTimeStatusTimer, this, &GrblJob::onStopRealTimeStatusTimer);
  connect(real_time_status_timer_, &QTimer::timeout, this, &GrblJob::onStatusReport);

  timeout_timer_ = new QTimer(this);
  connect(this, &GrblJob::startTimeoutTimer, this, &GrblJob::onStartTimeoutTimer);
  connect(this, &GrblJob::stopTimeoutTimer, this, &GrblJob::onStopTimeoutTimer);
  connect(timeout_timer_, &QTimer::timeout, this, &GrblJob::timeout);

#ifdef ENABLE_STATUS_REPORT
  is_starting_report_timer_ = false;
  realtime_status_report_timer_ = new QTimer(this);
  realtime_status_report_timer_->setSingleShot(true);
  connect(realtime_status_report_timer_, &QTimer::timeout, this, &GrblJob::onTimeToGetStatus);
#endif

}

GrblJob::GrblJob(QObject *parent, QString endpoint, QVariant &&gcode) :
     BaseJob(parent, endpoint, gcode) {
  gcode_ = gcode.toStringList();

  //connect(&(SerialPort::getInstance()), &SerialPort::responseReceived, this, &GrblJob::parseResponse);
  connect(&(SerialPort::getInstance()), &SerialPort::responseReceived, this, &GrblJob::onResponseReceived);
  connect(&(SerialPort::getInstance()), &SerialPort::disconnected, this, &GrblJob::onPortDisconnected);

  real_time_status_timer_ = new QTimer(this);
  connect(this, &GrblJob::startRealTimeStatusTimer, this, &GrblJob::onStartRealTimeStatusTimer);
  connect(this, &GrblJob::stopRealTimeStatusTimer, this, &GrblJob::onStopRealTimeStatusTimer);
  connect(real_time_status_timer_, &QTimer::timeout, this, &GrblJob::onStatusReport);

  timeout_timer_ = new QTimer(this);
  connect(this, &GrblJob::startTimeoutTimer, this, &GrblJob::onStartTimeoutTimer);
  connect(this, &GrblJob::stopTimeoutTimer, this, &GrblJob::onStopTimeoutTimer);
  connect(timeout_timer_, &QTimer::timeout, this, &GrblJob::timeout);

#ifdef ENABLE_STATUS_REPORT
  is_starting_report_timer_ = false;
  realtime_status_report_timer_ = new QTimer(this);
  realtime_status_report_timer_->setSingleShot(true);
  connect(realtime_status_report_timer_, &QTimer::timeout, this, &GrblJob::onTimeToGetStatus);
#endif

}

GrblJob::~GrblJob() {
  mutex_.lock();
  mutex_.unlock();
  wait(wait_timeout_);
}

void GrblJob::start() {
  const QMutexLocker locker(&mutex_);
  if (!isRunning()) {
    QThread::start();
  } else {
    Q_ASSERT_X(false, "GrblJob", "Already running");
  }
}

int GrblJob::progress() {
  return progress_value_;
}

bool GrblJob::isPaused() {
  return status_ == Status::PAUSED;
}

/**
 * @brief Send the pause cmd
 */
void GrblJob::pause() {
  qInfo() << "[SerialPort] Write Pause (!)";
  SerialPort::getInstance().write_some("!");
  setStatus(Status::PAUSED); // Change status immediately
}

/**
 * @brief Send the resume cmd
 */
void GrblJob::resume() {
  qInfo() << "[SerialPort] Write Resume (~)";
  SerialPort::getInstance().write_some("~");
  setStatus(Status::RUNNING); // Change status immediately
}

/**
 * @brief Send the grbl reset cmd
 */
void GrblJob::stop() {
  qInfo() << "[SerialPort] Write Reset";
  SerialPort::getInstance().write_some("\x18");
  emit startTimeoutTimer(5000); // set a timeout in case the machine stops responding
}

/**
 * @brief Main loop for job execution
 *        Inherited from QThread
 */
void GrblJob::run() {
  mutex_.lock();

  // 1. Check whether the serial port is open (connected)
  if (!SerialPort::getInstance().isConnected()) {
    emit error(tr("Serial port not connected"));
    mutex_.unlock();
    setStatus(Status::STOPPED);
    return;
  }

  // 2. Poll until Grbl is ready
  setStatus(Status::STARTING);
  systemCmdNonblockingSend(SystemCmd::kUnlock);
  waiting_first_ok_ = true;
  auto wait_cnt = 4;
  bool is_ready = false;
  while (1) {
    while (!rcvd_lines_.empty()) {
      if (rcvd_lines_.front().contains(QString{"ok"}) ) {
        is_ready = true;
        break;
      }
      rcvd_lines_.pop_front();
    }
    if (is_ready) {
      break;
    }
    sleep(1);
    wait_cnt--;
    if (wait_cnt == 0) {
      SerialPort::getInstance().write_some("\n");
      wait_cnt = 3;
    }
  }
  rcvd_lines_.clear();

  // Start streaming g-code to grbl
  try {
    emit startRealTimeStatusTimer(1000);
    setStatus(Status::RUNNING);
    msleep(500);
    uint32_t l_count = 0;     // sent line count
    uint32_t error_count = 0; // error count
    uint32_t g_count = 0;     // acked gcode count (ok + error)
    // Send g-code program via a more agressive streaming protocol that forces characters into
    // Grbl's serial read buffer to ensure Grbl has immediate access to the next g-code command
    // rather than wait for the call-response serial protocol to finish. This is done by careful
    // counting of the number of characters sent by the streamer to Grbl and tracking Grbl's 
    // responses, such that we never overflow Grbl's serial read buffer. 
    // c_line: compact cmd line in buffer
    //         including the cmd to be processed by grbl and the next cmd to be sent
    QList<int> c_line;
    for (auto& line: gcode_) {
      // Strip comments/spaces/new line and capitalize
      QRegularExpression regex("\\s|\\(.*?\\)");
      QString l_block{line};
      l_block.replace(regex, QString{""});
      l_block = l_block.section(';', 0, 0);
      if (l_block.length() == 0) { // Discard empty line
        continue;
      }
      c_line.append(l_block.length()+1); // Track number of characters in grbl serial read buffer
      l_count += 1; // Iterate line counter
      // Block until any cmd is ACKed and buffer is available
      while (std::accumulate(c_line.begin(), c_line.end(), 0) >= (serial_buffer_size_-1)
              || rcvd_lines_.size() > 0
              || timeout_occurred_
              || port_disconnected_) {
        if (port_disconnected_) {
          setStatus(Status::STOPPED);
          throw "STOPPED PORT_DISCONNECTED";
        } else if (timeout_occurred_) {
          setStatus(Status::STOPPED);
          throw "STOPPED TIMEOUT";
        }
        if (rcvd_lines_.empty()) {
          continue;
        }
        QString out_temp = rcvd_lines_.front().trimmed(); // One line of grbl response
        if (out_temp.contains(QString{"ok"}) || out_temp.contains(QString{"error"})) {
          if (out_temp.contains("error")) {
            error_count += 1;
          }
          g_count += 1; // Iterate g-code counter
          //if verbose: print "  REC<"+str(g_count)+": \""+out_temp+"\""
          qInfo() << "REC<" << g_count << ": " + out_temp;
          c_line.pop_front(); //  Delete the block character count corresponding to the last 'ok'
        } else if (out_temp.startsWith("<")) {
          qInfo() << "rt status: " << out_temp;
          status_report_blocked_ = false;
          auto len = out_temp.indexOf('>') - 1;
          auto extracted = out_temp.mid(1, len > 0 ? len : 0);
          auto status_tokens = extracted.split('|', Qt::SkipEmptyParts);
          handleStatusReport(status_tokens);
          if (status() == Status::ALARM) {
            throw "ALARM";
          }
        } else if (out_temp.startsWith("ALARM:")) {
          qInfo() << out_temp;
          QStringRef subString = out_temp.midRef(sizeof("ALARM:"));
          int code = subString.toInt();
          if (code >= static_cast<int>(AlarmCode::kMin) &&
              code <= static_cast<int>(AlarmCode::kMax) ) {
            last_alarm_code_ = static_cast<AlarmCode>(code);
          }
          setStatus(Status::ALARM);
          throw "ALARM";
        } else if (out_temp.startsWith("Grbl ")) {
          qInfo() << out_temp;
          // Detected a grbl reset -> abort
          setStatus(Status::STOPPED);
          throw "STOPPED GRBL_RESET";
        } else if (out_temp.startsWith("[MSG:")) {
          qInfo() << out_temp;
        } else if (out_temp.startsWith("[echo:")) {
          qInfo() << out_temp;
        } else if (out_temp.startsWith("[HLP:")) {
          qInfo() << out_temp;
        } else if (out_temp.startsWith("[PRB:")) {
          qInfo() << out_temp;
        } else if (out_temp.startsWith("[VER:")) {
          qInfo() << out_temp;
        } else if (out_temp.startsWith("[OPT:")) {
          qInfo() << out_temp;
        } else if (out_temp.startsWith("[GC:")) {
          qInfo() << out_temp;
        } else if (out_temp.startsWith("[G:")) {
          qInfo() << out_temp;
        } else if (out_temp.startsWith("[TLO:")) {
          qInfo() << out_temp;
        } else {
          // Unknown msg
          qInfo() << "Unknown MSG: " + out_temp; // Debug response
        }
        rcvd_lines_.pop_front();
      }
      // Send the cmd
      qInfo() << "SND>" << l_count << ": " + l_block;
      SerialPort::getInstance().write_some(l_block.toStdString() + '\n'); // Send g-code block to grbl
      //if verbose: print "SND>"+str(l_count)+": \"" + l_block + "\""
      incFinishedCmdCnt();
      // Approach 1: progress_value_ = (int)(100 * current_line_ / gcode_.size());
      // Approach 2:
      if (QTime{0, 0}.secsTo(getTotalRequiredTime()) == 0) {
        progress_value_ = 100;
      } else {
        progress_value_ = 100 * QTime{0, 0}.secsTo(getElapsedTime()) / QTime{0, 0}.secsTo(getTotalRequiredTime());
      }
      emit progressChanged(progress_value_);

    }

    // All GCode have been sent, wait until all responses have been received.
    while (l_count > g_count) {
      if (port_disconnected_) {
        setStatus(Status::STOPPED);
        throw "STOPPED PORT_DISCONNECTED";
      } else if (timeout_occurred_) {
        setStatus(Status::STOPPED);
        throw "STOPPED TIMEOUT";
      }
      if (rcvd_lines_.empty()) {
        continue;
      }
      QString out_temp = rcvd_lines_.front().trimmed(); // Wait for grbl response
      if (out_temp.contains(QString{"ok"}) || out_temp.contains(QString{"error"})) {
        if (out_temp.contains("error")) {
          error_count += 1;
        }
        g_count += 1;       // Iterate g-code counter
        c_line.pop_front(); // Delete the block character count corresponding to the last 'ok'
        //if verbose: print "  REC<"+str(g_count)+": \""+out_temp + "\""
        qInfo() << "REC<" << g_count << ": \"" + out_temp + "\"";
      } else if (out_temp.startsWith("<")) {
        qInfo() << "rt status: " << out_temp;
        status_report_blocked_ = false;
        auto len = out_temp.indexOf('>') - 1;
        auto extracted = out_temp.mid(1, len > 0 ? len : 0);
        auto status_tokens = extracted.split('|', Qt::SkipEmptyParts);
        handleStatusReport(status_tokens);
        if (status() == Status::ALARM) {
          throw "ALARM";
        }
      } else if (out_temp.startsWith("ALARM:")) {
        QStringRef subString = out_temp.midRef(sizeof("ALARM:"));
        int code = subString.toInt();
        if (code >= static_cast<int>(AlarmCode::kMin) &&
            code <= static_cast<int>(AlarmCode::kMax) ) {
          last_alarm_code_ = static_cast<AlarmCode>(code);
        }
        setStatus(Status::ALARM);
        throw "ALARM";
      } else if (out_temp.startsWith("Grbl ")) {
        // Detected a grbl reset -> abort
        setStatus(Status::STOPPED);
        throw "STOPPED GRBL_RESET";
      } else if (out_temp.startsWith("[MSG:")) {
        qInfo() << "    MSG: \"" + out_temp + "\"";
      } else {
        // Unknown msg
        qInfo() << "    MSG: \"" + out_temp + "\""; // Debug response
      }
      rcvd_lines_.pop_front();
    }

    /*
    setStatus(Status::STARTING);
    // if the last job is terminated by an alarm, it will require an unlock
    systemCmdBlockingSend(systemCmd::kUnlock);

    systemCmdBlockingSend(systemCmd::kHelpMsg);
    //unprocssed_response_.clear();
    SerialPort::getInstance().clear_buf();
    //serial_->clear_buf();
    systemCmdBlockingSend(systemCmd::kBuildInfo);

    setStatus(Status::RUNNING);
    grbl_reset_condition_detected_ = false;

    // Loop sending
    current_line_ = 0;
    planner_block_unexecuted_count_ = 0;
    while (current_line_ < gcode_.size()) {
      // handle action request flags
      if (pause_flag_) {
        if (status() == Status::RUNNING) {
          qInfo() << "[SerialPort] Write Pause (!) (M5!?)";
          ctrlCmdNonblockingSend(ctrlCmd::kPause);
          setStatus(Status::PAUSED);
        }
        pause_flag_ = false;
      } else if (resume_flag_) {
        if (status() == Status::PAUSED) {
          qInfo() << "[SerialPort] Write ~";
          ctrlCmdNonblockingSend(ctrlCmd::kResume);
          setStatus(Status::RUNNING);
        }
        resume_flag_ = false;
      } else if (stop_flag_) {
        if (status() != Status::STOPPING && status() != Status::ERROR_STOPPING) {
          qInfo() << "[SerialPort] Serial Port Stop";
          ctrlCmdNonblockingSend(ctrlCmd::kReset);
          // Not change status here
          // change status when alarm is received
        }
        stop_flag_ = false;
      }

      // handle status flags

      if (last_alarm_code_ == 3 || last_alarm_code_ == 6) {
        if (status() != Status::STOPPING) {
          // user cancel (3 for normal motion, 6 for homing cycle)
          // Not an error
          setStatus(Status::STOPPING);
          startTimer(5000); // set timeout for any transient state
          // expect/wait for a grbl reset (hello) msg
        }
      }
      if (last_alarm_code_ >= 7 && last_alarm_code_ <= 9) {
        if (status() != Status::ERROR_STOPPING) {
          emit error(tr("Homing failed"));
          setStatus(Status::ERROR_STOPPING); // wait for grbl reset msg
          startTimer(5000); // set timeout for any transient state
          // expect/wait for a grbl reset (hello) msg
        }
      }

      if (grbl_reset_condition_detected_) {
        grbl_reset_condition_detected_ = false;
        if (status() == Status::ERROR_STOPPING) {
          //emit error(tr("GRBL reset"));
          throw "ERROR_STOPPED GRBL_RESET";
        } else if (status() == Status::STOPPING) {
          throw "STOPPED USER_ABORT";
        } else if (status() == Status::PAUSED){
          setStatus(Status::STOPPING);
          throw "STOPPED USER_ABORT";
        }
      }
      if (timeout_occurred_) { // No response
        // send a force stop cmd in case the machine is still working?
        //qInfo() << "Send a force stop cmd when no response";
        //serial_->write_some("\x18");
        emit error(tr("Machine no response"));
        setStatus(Status::ERROR_STOPPED); // or ERROR_STOPPING ?
        throw "ERROR_STOPPED NO_RESPONSE";
      }

      // Stop sending the next gcode if not in valid state
      if (status() != Status::RUNNING) {
        continue;
      }

      // Don't execute the following if waiting for response 
      // (i.e. Any previous cmd hasn't been digested)
      if (system_cmd_state_.cmd != systemCmd::kNull) {
        continue;
      }
      if (gcode_cmd_comm_state_ != gcodeCmdCommState::kIdle) {
        continue;
      }
      if (ctrl_cmd_state_.cmd != ctrlCmd::kNull) {
        continue;
      }

      // NOT USED. "ok" resp will be blocked automatically by grbl when planner block is full
      //           No need to acquire available number of planner block
      //if (plannerBufferFull()) {
      //  // WARNING: make sure bit(1) of $10 setting has been set
      //  //          to enable report of buffer cnt
      //  if (ctrl_cmd_state_.cmd == ctrlCmd::kRealTimeStatusReport) {
      //    continue;
      //  }
      //  if ( ! realtime_status_report_timer_->isActive() && !is_starting_report_timer_) {
      //    startStatusPolling();
      //    is_starting_report_timer_ = true;
      //  }
      //  continue;
      //}

      // Send the next gcode
      const QByteArray data = QString(gcode_[current_line_] + "\n").toUtf8();
      qInfo() << "[SerialPort] Write" << gcode_[current_line_];
      if (data.startsWith("$H")) {
        systemCmdNonblockingSend(systemCmd::kHoming);
      } else {
        gcodeCmdNonblockingSend(data.toStdString());
      }

      current_line_++;
      //progress_value_ = (int)(100 * current_line_ / gcode_.size());
      if (QTime{0, 0}.secsTo(getTotalRequiredTime()) == 0) {
        progress_value_ = 100;
      } else {
        progress_value_ = 100 * QTime{0, 0}.secsTo(getElapsedTime()) / QTime{0, 0}.secsTo(getTotalRequiredTime());
      }
      emit progressChanged(progress_value_);
    }

    while (1) {
      // TODO: Set a timeout
      //Wait for ok of the last cmd
      if (system_cmd_state_.cmd != systemCmd::kNull) {
        continue;
      }
      if (gcode_cmd_comm_state_ != gcodeCmdCommState::kIdle) {
        continue;
      }
      if (ctrl_cmd_state_.cmd != ctrlCmd::kNull) {
        continue;
      }
      break;
    }
    qInfo() << "[SerialPort] All sent - closing";

#ifdef ENABLE_STATUS_REPORT
    // Polling unexecuted block count until 0
    qInfo() << "[SerialPort] All sent - waiting until all motions finish";
    while (planner_block_unexecuted_count_ > 0) {
      // TODO: Set a timeout for unexpected no response
      if (ctrl_cmd_state_.cmd == ctrlCmd::kRealTimeStatusReport) {
        continue;
      }
      if ( ! realtime_status_report_timer_->isActive() && !is_starting_report_timer_) {
        startStatusPolling();
        is_starting_report_timer_ = true;
      }
    }
#endif
    */
    progress_value_ = 100;
    setStatus(Status::FINISHED);
    emit stopRealTimeStatusTimer();
    // Possible final state: FINISHED
    mutex_.unlock();
  } catch (...) { // const std::exception& e
    // NOTE: error signal has been emitted before throwing exception
    progress_value_ = 0;
    emit stopTimeoutTimer();
    emit stopRealTimeStatusTimer();
    if (status() == Status::ALARM) {
      setStatus(Status::ALARM_STOPPED);
    } else if (status() != Status::STOPPED){
      setStatus(Status::STOPPED);
    }
    // Possible final state: ALARM_STOPPED or STOPPED
    mutex_.unlock();
  }
}
/*
void GrblJob::receive() {
  QByteArray new_data = serial_->readAll();
  qInfo() << "[SerialPort] Receive data" << new_data;
  QByteArray resp_data = unprocssed_response_ + new_data;
  QString resp_str = QString::fromUtf8(resp_data);
  int processed_chars = 0;
  for (int i = 0; i < resp_str.length(); i++) {
    if (resp_str[i] == '\n') {
      parseResponse(resp_str.right(resp_str.length() - processed_chars).left(i - processed_chars));
      processed_chars = i + 1;
    }
  }
  unprocssed_response_ = resp_data.right(resp_data.length() - processed_chars);
}
*/

/**
 * @brief Parse a complete line of response
 * @param line a line of response
 */
 /*
void GrblJob::parseResponse(QString line) {
  if (line.isEmpty()) return;
  qInfo() << "[SerialPort] Receive line" << line;
  if (line == "ok" || line == "ok\r") {
    if (waiting_first_ok_) waiting_first_ok_ = false;

    // clear flags
    if (system_cmd_state_.cmd != systemCmd::kNull) {
      system_cmd_state_.cmd = systemCmd::kNull;
      system_cmd_state_.comm_state = systemCmdCommState::kIdle;
    }
    if (gcode_cmd_comm_state_ != gcodeCmdCommState::kIdle) {
      gcode_cmd_comm_state_ = gcodeCmdCommState::kIdle;
      incFinishedCmdCnt();
      stopTimer();
      emit elapsedTimeChanged(getElapsedTime());
    }
  } else if (line.startsWith("error:")) { // invalid gcode
    // TODO: terminate when any invalid gcode is detected

  } else if (line.startsWith("[HLP:$$")) { // system cmd $ (help msg) resp
    system_cmd_state_.cmd = systemCmd::kHelpMsg;
    system_cmd_state_.comm_state = systemCmdCommState::kWaitingOk;
    qInfo() << "[SerialPort] Grbl is ready";
  } else if (line.startsWith("[MSG:Restoring spindle")) {

  } else if (line.startsWith("[VER:")) {// 1st resp of $i (e.g. [VER:1.1f.20180715:])
    // parse grbl build info
  } else if (line.startsWith("[OPT")) { // 2nd resp of $i (e.g. [OPT:VML,35,254])
    // get size of planner buffer and serial buffer
    QRegExp rx("(\\d+),(\\d+)");
    if ((rx.indexIn(line, 0)) != -1) {
      planner_total_block_count_ = rx.cap(1).toInt();
      serial_buffer_size_ = rx.cap(2).toInt();
    }
    system_cmd_state_.cmd = systemCmd::kBuildInfo;
    system_cmd_state_.comm_state = systemCmdCommState::kWaitingOk;
  } else if (line.startsWith("<")) { // real-time status report ('?' ctrl cmd)
    // <....|Bf:xx,yy|....>
    QRegExp rx("Bf:(\\d+),(\\d+)");
    if ((rx.indexIn(line, 0)) != -1) {
      int available_planner_cnt = rx.cap(1).toInt();
      qInfo() <<"Update planner available cnt: " << available_planner_cnt;
      planner_block_unexecuted_count_ = planner_total_block_count_ - available_planner_cnt;
#ifdef ENABLE_STATUS_REPORT
      if ( ! plannerBufferFull()) {
        stopStatusPolling();
      }
#endif
    }
    if (ctrl_cmd_state_.cmd == ctrlCmd::kRealTimeStatusReport) {
      ctrl_cmd_state_.cmd = ctrlCmd::kNull;
      ctrl_cmd_state_.comm_state = ctrlCmdCommState::kIdle;
    }
  } else if (line.startsWith("ALARM:")) {
    QStringRef subString(&line, 6, 1);
    last_alarm_code_ = subString.toInt();
  } else if (line.startsWith("Grbl ")) { // grbl hello message (e.g. after a reset)
    if (waiting_first_ok_) {
      return;
    }
    // end the reset cmd (\x18) session only when the grbl hello message is rcvd
    if (ctrl_cmd_state_.cmd == ctrlCmd::kReset &&
        ctrl_cmd_state_.comm_state == ctrlCmdCommState::kWaitingResp) {
      // user reset/abort
      ctrl_cmd_state_.cmd = ctrlCmd::kNull;
      ctrl_cmd_state_.comm_state = ctrlCmdCommState::kIdle;
    }
    grbl_reset_condition_detected_ = true;
  } else {
    qInfo() << "[SerialPort] Unrecognized response";
  }
}
  */

void GrblJob::onStartTimeoutTimer(int ms) {
  timeout_timer_->start(ms);
}

void GrblJob::onStopTimeoutTimer() {
  timeout_timer_->stop();
}

void GrblJob::timeout() {
  timeout_timer_->stop();
  timeout_occurred_ = true;
}

void GrblJob::onStartRealTimeStatusTimer(int ms) {
  qInfo() << "onStartRealTimeStatusTimer";
  real_time_status_timer_->start(ms);
}

void GrblJob::onStopRealTimeStatusTimer() {
  qInfo() << "onStopRealTimeStatusTimer";
  real_time_status_timer_->stop();
}


void GrblJob::gcodeCmdNonblockingSend(std::string cmd) {
  if (cmd.empty()) return;
  // WARNING: MUST change comm state before send
  gcode_cmd_comm_state_ = GcodeCmdCommState::kWaitingResp;
  SerialPort::getInstance().write_some(cmd);
  //emit startWaiting(kGrblTimeout);
  planner_block_unexecuted_count_++; // only gcode cmd might be added to planner block
}

/*
void GrblJob::ctrlCmdNonblockingSend(ctrlCmd cmd) {
  std::string cmd_str;
  switch (cmd) {
    case ctrlCmd::kRealTimeStatusReport:
      cmd_str = "?";
      ctrl_cmd_state_.cmd = cmd;
      ctrl_cmd_state_.comm_state = ctrlCmdCommState::kWaitingResp;
      break;
    case ctrlCmd::kReset:
      cmd_str = "\x18";
      ctrl_cmd_state_.cmd = cmd;
      ctrl_cmd_state_.comm_state = ctrlCmdCommState::kWaitingResp; // wait for grbl hello msg
      break;
    case ctrlCmd::kPause:
      cmd_str = "!";
      // no resp expected
      ctrl_cmd_state_.cmd = ctrlCmd::kNull;
      ctrl_cmd_state_.comm_state = ctrlCmdCommState::kIdle;
      break;
    case ctrlCmd::kResume:
      cmd_str = "~";
      // no resp expected (might resp a [MSG: Restoring spindle] but not always)
      ctrl_cmd_state_.cmd = ctrlCmd::kNull;
      ctrl_cmd_state_.comm_state = ctrlCmdCommState::kIdle;
      break;
    default:
      break;
  }
  if (cmd_str.empty()) {
    return;
  }
  SerialPort::getInstance().write_some(cmd_str);
  //serial_->write_some(cmd_str);
}
 */


void GrblJob::systemCmdNonblockingSend(SystemCmd cmd) {
  std::string cmd_str;
  switch (cmd) {
    case SystemCmd::kHelpMsg:
      cmd_str = "$\n";
      break;
    case SystemCmd::kBuildInfo:
      cmd_str = "$I\n";
      break;
    case SystemCmd::kGrblSettings:
      cmd_str = "$$\n";
      break;
    case SystemCmd::kHoming:
      cmd_str = "$H\n";
      break;
    case SystemCmd::kUnlock:
      cmd_str = "$X\n";
      break;
    default:
      break;
  }
  if (cmd_str.empty()) {
    return;
  }
  system_cmd_state_.cmd = cmd;
  system_cmd_state_.comm_state = SystemCmdCommState::kWaitingResp;
  SerialPort::getInstance().write_some(cmd_str);
}

/*
void GrblJob::systemCmdBlockingSend(SystemCmd cmd) {
  std::string cmd_str;
  int timeout = kGrblTimeout; //default
  switch (cmd) {
    case SystemCmd::kHelpMsg:
      cmd_str = "$\n";
      break;
    case SystemCmd::kBuildInfo:
      cmd_str = "$I\n";
      break;
    case SystemCmd::kGrblSettings:
      cmd_str = "$$\n";
      break;
    case SystemCmd::kHoming:
      cmd_str = "$H\n";
      timeout = 30000;
      break;
    case SystemCmd::kUnlock:
      cmd_str = "$X\n";
      break;
    default:
      break;
  }
  if (cmd_str.empty()) {
    return;
  }
  system_cmd_state_.cmd = cmd;
  system_cmd_state_.comm_state = SystemCmdCommState::kWaitingResp;
  SerialPort::getInstance().write_some(cmd_str);

  startTimer(timeout);
  while (system_cmd_state_.cmd != SystemCmd::kNull && !timeout_occurred_) {
    msleep(500);
  }
  if (timeout_occurred_) {
    emit error(tr("GRBL system cmd timeout"));
    throw "cmd timeout";
  }
  stopTimer();
}
 */

void GrblJob::onPortDisconnected() {
  //qInfo() << "Port disconnected";
  port_disconnected_ = true;
}

void GrblJob::onResponseReceived(QString line) {
  rcvd_lines_.push_back(line);
}

void GrblJob::handleStatusReport(const QStringList& tokens) {
  /**
   * STATE
   * |{MPos,WPos}:xxx,xxx,xxx       // position
   * [|Bf:xxx,xxx]                  // plan block buffer, serial buffer available count
   * [|Ln:xxx]                      // current line numbe
   * [|{FS:xxx,xxx,F:xxx}]          // spindle speed
   * [|Pn:[P][X][Y][Z][D][R][H][S]] // input/sensor pin state
   * [|WCO:xxx]                     // FIELD_WORK_COORD_OFFSET
   * [|Ov:XX,XX,XX[|A:{S,C}[F][M]]] // feed_ovr, rapid_ovr, spindle_ovr, spindle_state, fload_state, mist_state
   */
  if (tokens.empty()) {
    return;
  }
  Status new_status = status();
  if (tokens.at(0).startsWith("Idle")) {
    new_status = Status::READY;
  } else if (tokens.at(0).startsWith("Run")) {
    new_status = Status::RUNNING;
  } else if (tokens.at(0).startsWith("Hold")) {
    new_status = Status::PAUSED;
  } else if (tokens.at(0).startsWith("Home")) {
    new_status = Status::RUNNING;
  } else if (tokens.at(0).startsWith("Alarm")) {
    new_status = Status::ALARM;
  } else if (tokens.at(0).startsWith("Jog")) {
  } else if (tokens.at(0).startsWith("Check")) {
  } else if (tokens.at(0).startsWith("Door")) {
  } else if (tokens.at(0).startsWith("Sleep")) {
  } else {
    qInfo() << "Unknown state";
  }

  if (status() != new_status) {
    setStatus(new_status);
  }

}

void GrblJob::onStatusReport() {
  if (!status_report_blocked_ && SerialPort::getInstance().isConnected()) {
    qInfo() << "Send ?";
    status_report_blocked_ = true;
    SerialPort::getInstance().write_some("?");
  }
}

#ifdef ENABLE_STATUS_REPORT
void GrblJob::startStatusPolling() {
  realtime_status_report_timer_->start(500);
  is_starting_report_timer_ = false;
}

void GrblJob::stopStatusPolling() {
  realtime_status_report_timer_->stop();
}

void GrblJob::onTimeToGetStatus() {
  qInfo() << "Send ?";
  ctrlCmdNonblockingSend(ctrlCmd::kRealTimeStatusReport);
}
#endif
