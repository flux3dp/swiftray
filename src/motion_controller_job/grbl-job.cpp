#include <motion_controller_job/grbl-job.h>
#include <QIODevice>
#include <QMessageBox>
#include <QDebug>
#include <QSerialPortInfo>
#include <QTimer>
#include <QRegularExpression>
#include <numeric>
#include <QtMath>

#include <connection/serial-port.h>
#include <motion_controller_job/grbl-job.h>
#include <globals.h>

GrblJob::GrblJob(QObject *parent, QString endpoint, const QVariant &gcode) :
     BaseJob(parent, endpoint, gcode) {
  gcode_ = gcode.toStringList();

  connect(&serial_port, &SerialPort::lineReceived, this, &GrblJob::onResponseReceived);
  connect(&serial_port, &SerialPort::disconnected, this, &GrblJob::onPortDisconnected);


  real_time_status_timer_ = new QTimer(this);
  connect(this, &GrblJob::startRealTimeStatusTimer, this, &GrblJob::onStartRealTimeStatusTimer);
  connect(this, &GrblJob::stopRealTimeStatusTimer, this, &GrblJob::onStopRealTimeStatusTimer);
  connect(real_time_status_timer_, &QTimer::timeout, this, &GrblJob::onStatusReport);

  timeout_timer_ = new QTimer(this);
  connect(this, &GrblJob::startTimeoutTimer, this, &GrblJob::onStartTimeoutTimer);
  connect(this, &GrblJob::stopTimeoutTimer, this, &GrblJob::onStopTimeoutTimer);
  connect(timeout_timer_, &QTimer::timeout, this, &GrblJob::timeout);

}

GrblJob::GrblJob(QObject *parent, QString endpoint, QVariant &&gcode) :
     BaseJob(parent, endpoint, gcode) {
  gcode_ = gcode.toStringList();

  connect(&serial_port, &SerialPort::lineReceived, this, &GrblJob::onResponseReceived);
  connect(&serial_port, &SerialPort::disconnected, this, &GrblJob::onPortDisconnected);

  real_time_status_timer_ = new QTimer(this);
  connect(this, &GrblJob::startRealTimeStatusTimer, this, &GrblJob::onStartRealTimeStatusTimer);
  connect(this, &GrblJob::stopRealTimeStatusTimer, this, &GrblJob::onStopRealTimeStatusTimer);
  connect(real_time_status_timer_, &QTimer::timeout, this, &GrblJob::onStatusReport);

  timeout_timer_ = new QTimer(this);
  connect(this, &GrblJob::startTimeoutTimer, this, &GrblJob::onStartTimeoutTimer);
  connect(this, &GrblJob::stopTimeoutTimer, this, &GrblJob::onStopTimeoutTimer);
  connect(timeout_timer_, &QTimer::timeout, this, &GrblJob::timeout);

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
  serial_port.write(QString{"!"});
  setStatus(Status::PAUSED); // Change status immediately
}

/**
 * @brief Send the resume cmd
 */
void GrblJob::resume() {
  qInfo() << "[SerialPort] Write Resume (~)";
  serial_port.write(QString{"~"});
  setStatus(Status::RUNNING); // Change status immediately
}

/**
 * @brief Send the grbl reset cmd
 */
void GrblJob::stop() {
  qInfo() << "[SerialPort] Write Reset";
  serial_port.write(QString{"\x18"});
  emit startTimeoutTimer(5000); // set a timeout in case the machine stops responding
}

/**
 * @brief Main loop for job execution
 *        Inherited from QThread
 */
void GrblJob::run() {
  mutex_.lock();

  // 1. Check whether the serial port is open (connected)
  if (!serial_port.isOpen()) {
    emit error(tr("Serial port not connected"));
    mutex_.unlock();
    setStatus(Status::STOPPED);
    return;
  }

  try {
    // 2. Poll until Grbl is ready
    setStatus(Status::STARTING);
    systemCmdNonblockingSend(SystemCmd::kUnlock);
    waiting_first_ok_ = true;
    auto wait_countdown = 4;
    bool is_ready = false;
    while (1) {
      if (port_disconnected_) {
        setStatus(Status::STOPPED);
        throw "STOPPED PORT_DISCONNECTED";
      }
      sleep(1);
      rcvd_mutex_.lock();
      while (!rcvd_lines_.empty()) {
        if (rcvd_lines_.front().contains(QString{"ok"}) ) {
          is_ready = true;
          break;
        }
        rcvd_lines_.pop_front();
      }
      rcvd_mutex_.unlock();
      if (is_ready) {
        qInfo() << "grbl ready";
        break;
      }
      wait_countdown--;
      if (wait_countdown == 0) {
        // timeout
        emit error(tr("Serial port not responding"));
        setStatus(Status::STOPPED);
        throw "STOPPED TIMEOUT";
      } else {
        serial_port.write(QString{"\n"});
      }
    }
    rcvd_mutex_.lock();
    rcvd_lines_.clear();
    rcvd_mutex_.unlock();

    // Start streaming g-code to grbl
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
      incFinishedCmdCnt();
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
          emit error(tr("Serial port disconnected"));
          setStatus(Status::STOPPED);
          throw "STOPPED PORT_DISCONNECTED";
        } else if (timeout_occurred_) {
          emit error(tr("Serial port not responding"));
          setStatus(Status::STOPPED);
          throw "STOPPED TIMEOUT";
        }
        if (rcvd_lines_.empty()) {
          continue;
        }
        rcvd_mutex_.lock();
        Q_ASSERT_X(rcvd_lines_.size() > 0, "GRBL Job", "Receive buffer must has at least one item");
        QString out_temp = rcvd_lines_.front().trimmed(); // One line of grbl response
        rcvd_lines_.pop_front();
        rcvd_mutex_.unlock();
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
          handleRealtimeStatusReport(status_tokens);
          if (status() == Status::ALARM) {
            throw "ALARM";
          }
        } else if (out_temp.startsWith("ALARM:")) {
          qInfo() << out_temp;
          QString subString = out_temp.mid(strlen("ALARM:"));
          int code = subString.toInt();
          if (code >= static_cast<int>(AlarmCode::kMin) &&
              code <= static_cast<int>(AlarmCode::kMax) ) {
            last_alarm_code_ = static_cast<AlarmCode>(code);
          }
          if (last_alarm_code_ != AlarmCode::kAbortCycle) { // No need to show message in abort cycle alarm
            emit error(tr("Alarm code: ") + QString::number(static_cast<int>(last_alarm_code_)));
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
        } else if (out_temp.startsWith("[FLUX:")) {
          // ======== FLUX's dedicated response ========
          qInfo() << out_temp;
          if (out_temp.contains("act")) {
            emit error(tr("Machine is paused by drop or collision."));
          } else if (out_temp.contains("tilt")) {
            emit error(tr("Machine is paused by tilt."));
          }
          setStatus(Status::PAUSED);
        } else {
          // Unknown msg
          qInfo() << "Unknown MSG: " + out_temp; // Debug response
        }
      }
      // Send the cmd
      qInfo() << "SND>" << l_count << ": " << l_block;
      serial_port.write(l_block + '\n'); // Send g-code block to grbl
      //if verbose: print "SND>"+str(l_count)+": \"" + l_block + "\""
      // Approach 1: progress_value_ = (int)(100 * current_line_ / gcode_.size());
      // Approach 2:
      if (QTime{0, 0}.secsTo(getTotalRequiredTime()) == 0) {
        progress_value_ = 100;
      } else {
        qInfo() << getElapsedTime().toString("hh:mm:ss");
        progress_value_ = 100 * QTime{0, 0}.secsTo(getElapsedTime()) / QTime{0, 0}.secsTo(getTotalRequiredTime());
      }
      emit progressChanged(progress_value_);
      emit elapsedTimeChanged(getElapsedTime());

    }

    // All GCode have been sent, wait until all responses have been received.
    while (l_count > g_count) {
      if (port_disconnected_) {
        emit error(tr("Serial port disconnected"));
        setStatus(Status::STOPPED);
        throw "STOPPED PORT_DISCONNECTED";
      } else if (timeout_occurred_) {
        emit error(tr("Serial port not responding"));
        setStatus(Status::STOPPED);
        throw "STOPPED TIMEOUT";
      }
      if (rcvd_lines_.empty()) {
        continue;
      }
      rcvd_mutex_.lock();
      Q_ASSERT_X(rcvd_lines_.size() > 0, "GRBL Job", "Receive buffer must has at least one item");
      QString out_temp = rcvd_lines_.front().trimmed(); // Wait for grbl response
      rcvd_lines_.pop_front();
      rcvd_mutex_.unlock();
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
        handleRealtimeStatusReport(status_tokens);
        if (status() == Status::ALARM) {
          throw "ALARM";
        }
      } else if (out_temp.startsWith("ALARM:")) {
        QString subString = out_temp.mid(strlen("ALARM:"));
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
    }

    progress_value_ = 100;
    emit stopTimeoutTimer();
    emit stopRealTimeStatusTimer();
    // Possible final state: FINISHED
    disconnect(&serial_port, &SerialPort::lineReceived, this, &GrblJob::onResponseReceived);
    setStatus(Status::FINISHED);
    mutex_.unlock();
  } catch (...) { // const std::exception& e
    // NOTE: error signal has been emitted before throwing exception
    progress_value_ = 0;
    emit stopTimeoutTimer();
    emit stopRealTimeStatusTimer();
    disconnect(&serial_port, &SerialPort::lineReceived, this, &GrblJob::onResponseReceived);
    if (status() == Status::ALARM) {
      setStatus(Status::ALARM_STOPPED);
    } else if (status() != Status::STOPPED){
      setStatus(Status::STOPPED);
    }
    // Possible final state: ALARM_STOPPED or STOPPED
    mutex_.unlock();
  }
}

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
  serial_port.write(cmd);
  //emit startWaiting(kGrblTimeout);
  planner_block_unexecuted_count_++; // only gcode cmd might be added to planner block
}

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
  serial_port.write(cmd_str);
}

void GrblJob::onPortDisconnected() {
  //qInfo() << "Port disconnected";
  port_disconnected_ = true;
}

void GrblJob::onResponseReceived(QString line) {
  rcvd_mutex_.lock();
  rcvd_lines_.push_back(line);
  rcvd_mutex_.unlock();
}

void GrblJob::handleRealtimeStatusReport(const QStringList& tokens) {
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
  if (!status_report_blocked_ && serial_port.isOpen()) {
    qInfo() << "Send ?";
    status_report_blocked_ = true;
    serial_port.write(QString{"?"});
  }
}

/**
 * @brief Calculate the required time from the GCodes inside text area
 * @retval A list of timestamp corresponding to each line of GCode
 *         throw exception when canceled
 */
QList<QTime> GrblJob::calcRequiredTime(QStringList gcode_list, QProgressDialog* progress_dialog) {
  QList<QTime> timestamp_list;
  progress_dialog->setMaximum(gcode_list.size()-1);
  //QStringList gcode_list = ui->gcodeText->toPlainText().split('\n');
  int current_line = 0;
  //int g_motion_modal = 0;    // 0, 1, 2, 3, 80, 81, 82, 84, 85, 86, 87, 88, 89
  //int g_distance_modal = 90; // 90, 91
  bool relative_mode = false; // G90 or G91
  float last_abs_x = 0, last_abs_y = 0, last_abs_z = 0;
  float x_param = 0, y_param = 0, z_param = 0, f_param = 7500;

  QTime required_time{0, 0};

  bool canceled = false;
  connect(progress_dialog, &QProgressDialog::canceled, [&]() {
      canceled = true;
  });
  progress_dialog->setWindowModality(Qt::WindowModal);
  //progress.setWindowModality(Qt::NonModal);
  //progress.setWindowModality(Qt::ApplicationModal);
  progress_dialog->show();
  while (current_line < gcode_list.size()) {
    if (canceled) {
      throw "Canceled";
    }
    if (current_line % 100 == 0 || current_line == (gcode_list.size() - 1)) {
      progress_dialog->setValue(current_line);
    }
    QString line = gcode_list[current_line];
    line = line.toUpper().section(';', 0, 0); // Eliminate comment (;)
    if (line.startsWith("B", Qt::CaseSensitivity::CaseInsensitive) ||
        line.startsWith("D", Qt::CaseSensitivity::CaseInsensitive) ||
        line.startsWith("$", Qt::CaseSensitivity::CaseInsensitive)) {
      // do nothing (FLUX's custom cmd)
    } else {
      QChar current_param{';'};
      QString val_str;
      line.append(' '); // To ensure the last param is processed
      // Set default value when X/Y field is absent
      x_param = relative_mode ? 0 : last_abs_x;
      y_param = relative_mode ? 0 : last_abs_y;
      z_param = relative_mode ? 0 : last_abs_z;
      for (auto& c: line) {
        if (c == '-' || c == '.' || c.isDigit()) {
          if (current_param != ';') {
            val_str.append(c);
          }
        } else {
          // Finish a cmd (G-Code or M-Code)
          if (current_param == 'G') {
            if (val_str.toInt() == 90 || val_str.toInt() == 91) {
              relative_mode = val_str.toInt() == 90 ? false : true;
              // Reset default value when X/Y field is absent
              x_param = relative_mode ? 0 : last_abs_x;
              y_param = relative_mode ? 0 : last_abs_y;
              z_param = relative_mode ? 0 : last_abs_z;
            } else if (val_str.toInt() == 1) {
              // G1 Motion modal group
            }
          } else if (current_param == 'M') {
            // ignore
          }

          // Finish a param
          if (current_param == 'X') {
            x_param = val_str.toFloat();
          } else if (current_param == 'Y') {
            y_param = val_str.toFloat();
          } else if (current_param == 'Z') {
            z_param = val_str.toFloat();
          } else if (current_param == 'F') {
            f_param = val_str.toFloat();
          } else if (current_param == 'S') {
            // ignore
          }

          // The start of new param
          if (c == 'G' || c == 'M' || c == 'X' || c == 'Y' || c == 'Z' ||
              c == 'S' || c == 'F' ) {
            // Finish a param
            current_param = c;
            val_str.clear();
          } else {
            current_param = ';';
            val_str.clear();
          }
        }
      }

      /* GCode Analyze with Regex (time consuming)
      QRegularExpression re("((?<cmd>[GM])(?<cmd_idx>[\\d]+))?(?<param>([\\t ]*[XYZSEF][\\d.-]+)+)");
      QRegularExpressionMatch match = re.match(line);
      if ( ! match.captured("cmd").isEmpty() && match.captured("cmd")[0] == 'G') {
        if (match.captured("cmd_idx").toInt() >= 0 && match.captured("cmd_idx").toInt() <=3 ) {
          // G Motion modal group
        } else if (match.captured("cmd_idx").toInt() >= 90 && match.captured("cmd_idx").toInt() <= 91 ) {
          // G Distance modal group
        }
      }
      if ( ! match.captured("param").isEmpty()) {
        QRegularExpression param_re("[XYZSEF][\\d.-]+");
        QRegularExpressionMatch param_match = param_re.match(match.captured("param"));
      }
      */

      Q_ASSERT_X(f_param > 0, "GCode Player", "Feedrate must be larger than 0");
      // NOTE: F value is in unit of mm/min
      if (relative_mode) {
        required_time = required_time.addMSecs(
                1000 *
                qSqrt(qPow(x_param, 2) + qPow(y_param, 2) + qPow(z_param, 2)) /
                f_param *
                60);
        last_abs_x += x_param;
        last_abs_y += y_param;
        last_abs_z += z_param;
      } else {
        required_time = required_time.addMSecs(
                1000 *
                qSqrt(qPow(x_param-last_abs_x, 2) +
                      qPow(y_param-last_abs_y, 2) +
                      qPow(z_param-last_abs_z, 2)) /
                f_param *
                60);
        last_abs_x = x_param;
        last_abs_y = y_param;
        last_abs_z = z_param;
      }
    }

    timestamp_list << required_time;
    current_line++;
  }

  return timestamp_list;
}