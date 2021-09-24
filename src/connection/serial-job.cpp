#include <connection/serial-job.h>
#include <QIODevice>
#include <QMessageBox>
#include <QDebug>
#include <QSerialPortInfo>
#include <QTimer>

#include <SerialPort/SerialPort.h>

SerialJob::SerialJob(QObject *parent, QString endpoint, QVariant gcode) :
     BaseJob(parent, endpoint, gcode) {
  port_ = endpoint.split(":")[0];
  baudrate_ = endpoint.split(":")[1].toInt();
  gcode_ = gcode.toStringList();

  planner_total_block_count_ = 1;
  planner_block_unexecuted_count_ = 0;
  serial_buffer_size_ = 100;

  waiting_first_ok_ = true;
  timeout_occurred_ = false;
  grbl_reset_condition_detected_ = false;
  last_alarm_code_ = -1; // no alarm

  gcode_cmd_comm_state_ = gcodeCmdCommState::kIdle;
  ctrl_cmd_state_.cmd = ctrlCmd::kNull;
  ctrl_cmd_state_.comm_state = ctrlCmdCommState::kIdle;
  system_cmd_state_.cmd = systemCmd::kNull;
  system_cmd_state_.comm_state = systemCmdCommState::kIdle;

  serial_ = std::make_unique<SerialPort>();
  connect(serial_.get(), &SerialPort::responseReceived, this, &SerialJob::parseResponse);

  pause_flag_ = false;
  resume_flag_ = false;
  stop_flag_ = false;

  timeout_timer_ = new QTimer(this);
  connect(this, &SerialJob::startWaiting, this, &SerialJob::startTimer);
  connect(this, &SerialJob::waitComplete, this, &SerialJob::stopTimer);
  connect(timeout_timer_, &QTimer::timeout, this, &SerialJob::timeout);

#ifdef ENABLE_STATUS_REPORT
  is_starting_report_timer_ = false;
  realtime_status_report_timer_ = new QTimer(this);
  realtime_status_report_timer_->setSingleShot(true);
  connect(this, &SerialJob::startStatusPolling, this, &SerialJob::onStartStatusPolling);
  connect(this, &SerialJob::endStatusPolling, this, &SerialJob::onStopStatusPolling);
  connect(realtime_status_report_timer_, &QTimer::timeout, this, &SerialJob::onTimeToGetStatus);
#endif

}

SerialJob::~SerialJob() {
  mutex_.lock();
  mutex_.unlock();
  wait(wait_timeout_);
}

void SerialJob::start() {
  const QMutexLocker locker(&mutex_);
  if (!isRunning()) {
    QThread::start();
  } else {
    Q_ASSERT_X(false, "SerialJob", "Already running");
  }
}

int SerialJob::progress() {
  return progressValue_;
}

void SerialJob::pause() {
  pause_flag_ = true;
}

void SerialJob::resume() {
  resume_flag_ = true;
}

void SerialJob::stop() {
  stop_flag_ = true;
}

void SerialJob::run() {
  mutex_.lock();

  QString full_port_path;
  if (port_.startsWith("tty")) { // Linux/macOSX
    full_port_path += "/dev/";
    full_port_path += port_;
  } else { // Windows COMx
    full_port_path = port_;
  }
  qInfo() << "[SerialPort] Connecting" << port_ << baudrate_;
  bool rv = serial_->start(full_port_path.toStdString().c_str(), baudrate_);
  if (rv == false) {
    emit error(tr("Unable to connect serial port"));
    qWarning() << "[SerialPort] Failed to connect..";
    mutex_.unlock();
    setStatus(Status::ERROR_STOPPED);
    return;
  }
  qInfo() << "[SerialPort] Success connect!";

  //serial_->end_of_line_char('\n'); // not necessary
  //connect(serial_, &QSerialPort::readyRead, this, &SerialJob::receive);
  //unprocssed_response_.clear();

  // Poll until grbl is ready
  waiting_first_ok_ = true;
  serial_->write_some("\n"); // A 'ok' resp indicates grbl is ready
  auto wait_cnt = 4;
  while (waiting_first_ok_) {
    sleep(1);
    wait_cnt--;
    if (wait_cnt == 0) {
      serial_->write_some("\n"); // A 'ok' resp indicates grbl is ready
      wait_cnt = 4;
    }
  }

  try {
    // if the last job is terminated by an alarm, it will require an unlock
    systemCmdBlockingSend(systemCmd::kUnlock);

    systemCmdBlockingSend(systemCmd::kHelpMsg);
    //unprocssed_response_.clear();
    serial_->clear_buf();
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
          emit startWaiting(5000); // set timeout for any transient state
          // expect/wait for a grbl reset (hello) msg
        }
      }
      if (last_alarm_code_ >= 7 && last_alarm_code_ <= 9) {
        if (status() != Status::ERROR_STOPPING) {
          emit error(tr("Homing failed"));
          setStatus(Status::ERROR_STOPPING); // wait for grbl reset msg
          emit startWaiting(5000); // set timeout for any transient state
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
      if (system_cmd_state_.cmd != systemCmd::kNull) {
        continue;
      }
      if (gcode_cmd_comm_state_ != gcodeCmdCommState::kIdle) {
        continue;
      }
      if (ctrl_cmd_state_.cmd != ctrlCmd::kNull) {
        continue;
      }

      /* NOT USED. "ok" resp will be blocked automatically by grbl when planner block is full
       *           No need to acquire available number of planner block
      if (plannerBufferFull()) {
        // WARNING: make sure bit(1) of $10 setting has been set
        //          to enable report of buffer cnt
        if (ctrl_cmd_state_.cmd == ctrlCmd::kRealTimeStatusReport) {
          continue;
        }
        if ( ! realtime_status_report_timer_->isActive() && !is_starting_report_timer_) {
          emit startStatusPolling();
          is_starting_report_timer_ = true;
        }
        continue;
      }
      */

      // Send the next gcode
      const QByteArray data = QString(gcode_[current_line_] + "\n").toUtf8();
      if (data.startsWith("$H")) {
        systemCmdNonblockingSend(systemCmd::kHoming);
      } else {
        gcodeCmdNonblockingSend(data.toStdString());
      }

      qInfo() << "[SerialPort] Write" << gcode_[current_line_];

      current_line_++;
      progressValue_ = (int)(100 * current_line_ / gcode_.size());

      emit progressChanged();
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
        emit startStatusPolling();
        is_starting_report_timer_ = true;
      }
    }
#endif

    serial_->stop();
    mutex_.unlock();
    progressValue_ = 100;
    setStatus(Status::FINISHED);
  } catch (...) { // const std::exception& e
    // NOTE: error signal has been emitted before throwing exception
    progressValue_ = 0;
    if (status() == Status::ERROR_STOPPING) {
      setStatus(Status::ERROR_STOPPED);
    } else if (status() == Status::STOPPING) {
      setStatus(Status::STOPPED);
    }
    serial_->stop();
    mutex_.unlock();
  }
}
/*
void SerialJob::receive() {
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
void SerialJob::parseResponse(QString line) {
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
      emit waitComplete();
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
        emit endStatusPolling();
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

void SerialJob::startTimer(int ms) {
  timeout_timer_->start(ms);
  //timeout_timer_->start(kGrblTimeout);
}

void SerialJob::stopTimer() {
  timeout_timer_->stop();
}

void SerialJob::timeout() {
  timeout_timer_->stop();
  gcode_.clear();
  timeout_occurred_ = true;
}

void SerialJob::gcodeCmdNonblockingSend(std::string cmd) {
  if (cmd.empty()) return;
  serial_->write_some(cmd);
  gcode_cmd_comm_state_ = gcodeCmdCommState::kWaitingResp;
  emit startWaiting(kGrblTimeout);
  planner_block_unexecuted_count_++; // only gcode cmd might be added to planner block
}

void SerialJob::ctrlCmdNonblockingSend(ctrlCmd cmd) {
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
  serial_->write_some(cmd_str);
}

void SerialJob::systemCmdNonblockingSend(systemCmd cmd) {
  std::string cmd_str;
  switch (cmd) {
    case systemCmd::kHelpMsg:
      cmd_str = "$\n";
      break;
    case systemCmd::kBuildInfo:
      cmd_str = "$I\n";
      break;
    case systemCmd::kGrblSettings:
      cmd_str = "$$\n";
      break;
    case systemCmd::kHoming:
      cmd_str = "$H\n";
      break;
    case systemCmd::kUnlock:
      cmd_str = "$X\n";
      break;
    default:
      break;
  }
  if (cmd_str.empty()) {
    return;
  }
  system_cmd_state_.cmd = cmd;
  system_cmd_state_.comm_state = systemCmdCommState::kWaitingResp;
  serial_->write_some(cmd_str);
}


void SerialJob::systemCmdBlockingSend(systemCmd cmd) {
  std::string cmd_str;
  int timeout = kGrblTimeout; //default
  switch (cmd) {
    case systemCmd::kHelpMsg:
      cmd_str = "$\n";
      break;
    case systemCmd::kBuildInfo:
      cmd_str = "$I\n";
      break;
    case systemCmd::kGrblSettings:
      cmd_str = "$$\n";
      break;
    case systemCmd::kHoming:
      cmd_str = "$H\n";
      timeout = 30000;
      break;
    case systemCmd::kUnlock:
      cmd_str = "$X\n";
      break;
    default:
      break;
  }
  if (cmd_str.empty()) {
    return;
  }
  system_cmd_state_.cmd = cmd;
  system_cmd_state_.comm_state = systemCmdCommState::kWaitingResp;
  serial_->write_some(cmd_str);

  emit startWaiting(timeout);
  while (system_cmd_state_.cmd != systemCmd::kNull && !timeout_occurred_) {
    msleep(500);
  }
  if (timeout_occurred_) {
    emit error(tr("GRBL system cmd timeout"));
    throw "cmd timeout";
  }
  emit waitComplete();
}

#ifdef ENABLE_STATUS_REPORT
void SerialJob::onStartStatusPolling() {
  realtime_status_report_timer_->start(500);
  is_starting_report_timer_ = false;
}

void SerialJob::onStopStatusPolling() {
  realtime_status_report_timer_->stop();
}

void SerialJob::onTimeToGetStatus() {
  qInfo() << "Send ?";
  ctrlCmdNonblockingSend(ctrlCmd::kRealTimeStatusReport);
}
#endif
