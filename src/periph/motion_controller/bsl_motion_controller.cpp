#include "bsl_motion_controller.h"

#include <bitset>
#include <string>
#include <memory>
#include <thread>
#include <QDebug>
#include <QThread>
#include <executor/executor.h>
#include <debug/debug-timer.h>
#include <QApplication>
#include <QtCore/qcoreapplication.h>

#define MAX_BUFFER_LIST_SIZE 10000

// Time estimation constants
constexpr double Z_SPEED_IN_MM = 3;
constexpr double Z_PULSE_PER_MM = 1600;
constexpr double Z_SPEED_IN_PULSE = Z_SPEED_IN_MM * Z_PULSE_PER_MM;
constexpr double JUMP_SPEED = 4000;
constexpr int32_t JUMP_DELAY_MIN = 200;
constexpr int32_t JUMP_DELAY_MAX = 400;
constexpr double JUMP_DELAY = (double)(JUMP_DELAY_MIN+JUMP_DELAY_MAX)/2000;
constexpr int32_t LASER_ON_DELAY = -100;
constexpr int32_t LASER_OFF_DELAY = 100;
constexpr double LASER_DELAY = (double)(LASER_OFF_DELAY-LASER_ON_DELAY)/1000;

int lcs_error_ret = 0;
int lcs_error_count = 0;
uint32_t pos;
BoardRunStatus status;
ListStatus list_status;

LCS2Error doLcsApi(LCS2Error ret, QString cmd) {
  if (true || ret != LCS_RES_NO_ERROR) {
    qInfo() << "BSLM~::doLcsApi()" << cmd << ret;
  }
  return ret;
}

QString BSLMotionController::getErrorString(int error) {
  switch (error) {
    case LCS_RES_NO_ERROR:
      return "";
    case LCS_GENERAL_ACTION_FAILED:
      return "LCS_GENERAL_ACTION_FAILED";
    case LCS_GENERAL_ACTION_TIMEOUT:
      return "LCS_GENERAL_ACTION_TIMEOUT";
    case LCS_GENERAL_INVALID_PARAM:
      return "LCS_GENERAL_INVALID_PARAM";
    case LCS_GENERAL_OUT_OF_RANGE:
      return "LCS_GENERAL_OUT_OF_RANGE";
    case LCS_GENERAL_MEMORY_NOT_ENOUGH:
      return "LCS_GENERAL_MEMORY_NOT_ENOUGH";
    case LCS_GENERAL_BUFFER_SIZE_TOO_SMALL:
      return "LCS_GENERAL_BUFFER_SIZE_TOO_SMALL";
    case LCS_GENERAL_WRITE_ERROR:
      return "LCS_GENERAL_WRITE_ERROR";
    case LCS_GENERAL_READ_ERROR:
      return "LCS_GENERAL_READ_ERROR";
    case LCS_GENERAL_UUID_EXIST:
      return "LCS_GENERAL_UUID_EXIST";
    case LCS_GENERAL_UUID_NOT_EXIST:
      return "LCS_GENERAL_UUID_NOT_EXIST";
    case LCS_GENERAL_CURRENTLY_BUSY:
      return "LCS_GENERAL_CURRENTLY_BUSY";
    case LCS_GENERAL_PERMISSION_FAILED:
      return "LCS_GENERAL_PERMISSION_FAILED";
    case LCS_GENERAL_NOT_INITIALIZED:
      return "LCS_GENERAL_NOT_INITIALIZED";
    case LCS_GENERAL_NOT_OPENED:
      return "LCS_GENERAL_NOT_OPENED";
    case LCS_GENERAL_AREADY_OPENED:
      return "LCS_GENERAL_AREADY_OPENED";
    case LCS_BOARD_NOT_CONNECT:
      return "LCS_BOARD_NOT_CONNECT";
    default:
      return "Unknown LCS error";
  }
}

BSLMotionController::BSLMotionController(QObject *parent)
  : MotionController{parent}
{
  qInfo() << "BSLM~::BSLMotionController()";
  this->setState(MotionControllerState::kIdle);
}

BSLMotionController::~BSLMotionController() {
  qInfo() << this << "::~BSLMotionController()";
  setState(MotionControllerState::kQuit);
  if (this->command_runner_thread_.joinable()) {
    this->command_runner_thread_.join();
  }
  qInfo() << this << "::~BSLMotionController() - done";
}

void BSLMotionController::startCommandRunner() {
  qInfo() << this << "::startCommandRunner()";
  this->command_runner_thread_ = std::thread(&BSLMotionController::commandRunnerThread, this);
}

int debug_count_bsl  = 0;

void BSLMotionController::commandRunnerThread() {
  qInfo() << "BSLM~::thread() - entered @" << getDebugTime() << " - pending cmds.." << pending_cmds_.size();
  while (this->getState() != MotionControllerState::kQuit) {
    debug_count_bsl ++;
    switch (this->getState()) {
      case MotionControllerState::kPaused:
        if (!lcs_paused_) {
          // First loop after pause
          doLcsApi(lcs_pause_list(),"lcs_pause_list");
          lcs_paused_ = true;
          qInfo() << "SetStatus: Paused";
        }
        QThread::msleep(25); 
        if (debug_count_bsl % 40 == 1) {
          qInfo() << "BSLM~::thread() - paused";
        }
        break;
      case MotionControllerState::kSleep:
        // if (debug_count_bsl % 40 == 1) qInfo() << "BSLM~::thread() - Sleeping State";
        QThread::msleep(25); 
        break;
      case MotionControllerState::kAlarm:
        if (debug_count_bsl % 40 == 1) qInfo() << "BSLM~::thread() - Alarm State" << getErrorString(current_error_);
        QThread::msleep(25); 
        break;
      case MotionControllerState::kIdle:
      case MotionControllerState::kRun:
        if (lcs_paused_) { 
          // First loop after resume
          doLcsApi(lcs_restart_list(),"lcs_restart_list");
          lcs_paused_ = false;
          qInfo() << "SetStatus: Running";
          QThread::msleep(25); 
          qInfo() << "BSLM~::thread() - resuming";
        }
        qInfo() << "SetLock: try lock() in commandRunnerThread";
        this->cmd_list_mutex_.lock();
        qInfo() << "SetLock: lock() in commandRunnerThread";
        if (this->pending_cmds_.empty()) {
          if (debug_count_bsl % 40 == 1) {
            qInfo() << "BSLM~::thread() - No pending commands, wait 1s. Board Connection: " << isConnected() << "@" << getDebugTime();
            if (!isConnected()) {
              this->cmd_list_mutex_.unlock();
              qInfo() << "SetLock: unlock() in commandRunnerThread before setState quit empty";
              this->setState(MotionControllerState::kQuit); // Invalid this motion controller once the connection is lost
              break;
            }
          }
          this->cmd_list_mutex_.unlock();
          qInfo() << "SetLock: unlock() in commandRunnerThread before setState idle";
          this->should_flush_ = this->buffer_size_ > 0;
          setState(MotionControllerState::kIdle); // Set state to idle if there are no pending commands
          QThread::msleep(25);
        } else {
          if (debug_count_bsl % 1000 == 1) {
            qInfo() << "BSLM~::thread() - pending commands: " << this->pending_cmds_.size();
          }
          bool is_connected = isConnected();
          if (!is_connected) {
            this->cmd_list_mutex_.unlock();
            qInfo() << "SetLock: unlock() in commandRunnerThread before setState quit";
            this->setState(MotionControllerState::kQuit);
            break;
          }
          setState(MotionControllerState::kRun); // Set state to running if there are pending commands
          QString cmd = this->pending_cmds_.front();
          this->pending_cmds_.pop();
          this->cmd_list_mutex_.unlock();
          qInfo() << "SetLock: unlock() in commandRunnerThread before handleGcode";
          this->handleGcode(cmd);
          dequeueCmd(1);
        }
        break;
      case MotionControllerState::kCheck:
      case MotionControllerState::kUnknown:
      default:
        qInfo() << "BSLM~::thread() - Invalid state" << getDebugTime();
        setState(MotionControllerState::kAlarm);
        break;
    }
  }
}

void BSLMotionController::dequeueCmd(int count) {
  qInfo() << "SetLock: try lock() in dequeueCmd";
  this->cmd_list_mutex_.lock();
  qInfo() << "SetLock: lock() in dequeueCmd";
  for (int i = 0; i < count; i++) {
    if (!cmd_executor_queue_.isEmpty()) {
      auto exec = cmd_executor_queue_.at(0);
      exec->handleCmdFinish(0);
      dequeueCmdExecutor();
    }
  }
  this->cmd_list_mutex_.unlock();
  qInfo() << "SetLock: unlock() in dequeueCmd";
}

LCS2Error BSLMotionController::waitListAvailable(int list_no) {
  // qInfo() << "BSLM~::waitList(" << list_no << ")@" << getDebugTime();
  LCS2Error ret = doLcsApi(lcs_load_list(list_no, 0),"lcs_load_list" + QString::number(list_no));
  bool fixing_aready = false;
  while (ret != LCS_RES_NO_ERROR) {
    QThread::msleep(25);
    if (lcs_paused_) {
      continue;
    }
    qInfo() << getErrorString(ret);
    ret = doLcsApi(lcs_load_list(list_no, 0),"lcs_load_list" + QString::number(list_no));
    // If the list is already opened, close the list, execute it
    if (ret == LCS_GENERAL_AREADY_OPENED) {
      qInfo() << "BSLM~::waitListAvailable(" << list_no << ") - List already opened" << getDebugTime();
      if (!fixing_aready) {
        fixing_aready = true;
        if(!executeList(list_no)) return ret;
      }
      ret = doLcsApi(lcs_load_list(list_no, 0),"lcs_load_list" + QString::number(list_no));
    }
    if (lcs_error_count ++ > 100) {
      qWarning() << "BSLM~::waitListAvailable(" << list_no << ") - Error count exceeded 100" << getDebugTime();
      this->current_error_ = ret;
      this->stop();
      break;
    }
  }
  return ret;
}

void jump_to(double y, double x) {
  // If xy is inverted swap x, y
  doLcsApi(lcs_jump_abs(y, x),"lcs_jump_abs" + QString::number(y) + QString::number(x));
}

void mark_to(double y, double x) {
  // If xy is inverted, swap x, y
  doLcsApi(lcs_mark_abs(y, x),"lcs_mark_abs" + QString::number(y) + QString::number(x));
}

void BSLMotionController::handleGcode(const QString &gcode) {
    static TaskSettings settings;
    static bool rotary_mode = false;
    static bool laser_enabled = false;
    static bool is_absolute_positioning = true;
    static int list_no = 1;
    static double center_pos = 55;
    static int freq = 100; //100 khz
    static bool last_is_z_command = false;
    static int dotting_time = 0;
    static bool before_first_laser = true;
    static double wobble_k = 1;
    static QRegularExpression re("([GMXYFSZDWQPT]|WD|WS)(-?\\d+\\.?\\d*)");
    static QRegularExpressionMatchIterator i;

    // Skip these GCode
    if (gcode == "\u0018" || gcode == "$I" || gcode == "$H") {
        return;
    }

    if (gcode == "?" || gcode == "?\n") {
      Q_EMIT MotionController::statusUpdate(state_, x_pos_, y_pos_, 0);
      // qInfo() << "BSLM~::handleGcode() - Realtime status updated" << getDebugTime();
      return;
    }

    if (gcode.startsWith(";WOBBLE K", Qt::CaseSensitivity::CaseInsensitive)) {
        // Specific comment for Wobble
        wobble_k = gcode.mid(9).toFloat();
        return;
    }

    i = re.globalMatch(gcode);

    bool is_move_command = false;
    bool should_swap = false;
    bool should_end = false;
    double x = is_absolute_positioning ? x_pos_ : 0;
    double y = is_absolute_positioning ? y_pos_ : 0;
    double z = 0;

    QString command;

    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString type = match.captured(1);
        QString value = match.captured(2);
        
        if (type == "G") {
            command = type + value;
        } else if (type == "M") {
            command = type + value;
        } else if (type == "X") {
            x = value.toDouble();
            is_move_command = true;
        } else if (type == "Y") {
            y = value.toDouble();
            is_move_command = true;
        } else if (type == "Z") {
            z = value.toDouble();
            is_move_command = true;
        } else if (type == "Q") {
            freq = value.toInt();
            settings.period = 1000 / freq;
            doLcsApi(lcs_set_laser_pulses(settings.period, 0, settings.pulse_width),"lcs_set_laser_pulses");
          } else if (type == "P") {
            settings.pulse_width = value.toInt();
            doLcsApi(lcs_set_laser_pulses(settings.period, 0, settings.pulse_width),"lcs_set_laser_pulses");
        } else if (type == "T") {
            dotting_time = value.toInt();
        } else if (type == "F") {
            settings.current_f = value.toDouble() / 60;
            doLcsApi(lcs_set_mark_speed(settings.current_f),"lcs_set_mark_speed");
        } else if (type == "S") {
            settings.current_s = value.toInt();
            if (!is_handling_high_speed_) {
                if (settings.current_s > 0) {
                  laser_enabled = true;
                  doLcsApi(lcs_set_laser_power(settings.current_s / 10),"lcs_set_laser_power"); // Assuming S1000 is 100% power
                  if (before_first_laser) {
                    should_flush_ = true;
                    before_first_laser = false;
                  }
                } else {
                  laser_enabled = false;
                }
            }
        } else if (type == "W") {
            double workarea = value.toDouble();
            center_pos = workarea / 2;
        } else if (type == "D") {
            if (value == "0") {
                QChar resolution = gcode.at(3);
                if (resolution == 'U') high_speed_step_ = 0.025;
                else if (resolution == 'H') high_speed_step_ = 0.05;
                else if (resolution == 'L') high_speed_step_ = 0.2;
                else high_speed_step_ = 0.1;
            } else if (value == "1") {
                high_speed_data_.clear();
                high_speed_data_count_ = gcode.last(gcode.size() - 4).toInt();  // 4 = Prefix D1PC
                qInfo() << "BSLM~::handleGcode() - High speed data: " << high_speed_data_count_;
            } else if (value == "2") {
                high_speed_data_ += gcode.last(gcode.size() - 3).simplified();  // 3 = Prefix D2W
            } else if (value == "4") {
                is_handling_high_speed_ = true;
            }
            return;
        } else if (type == "WS") {
            settings.wobble_step = value.toDouble();
            if (settings.wobble_step > 0 && settings.wobble_diameter > 0) {
                lcs_set_wobble_mode(settings.wobble_diameter,
                                    settings.wobble_diameter,
                                    settings.wobble_step,
                                    WobbleType::WT_WHEEL);
            } else {
                lcs_set_wobble_mode(0, 0, 0, WobbleType::WT_DISABLE);
            }
        } else if (type == "WD") {
            settings.wobble_diameter = value.toDouble();
            if (settings.wobble_step > 0 && settings.wobble_diameter > 0) {
                lcs_set_wobble_mode(settings.wobble_diameter,
                                    settings.wobble_diameter,
                                    settings.wobble_step,
                                    WobbleType::WT_WHEEL);
            } else {
                lcs_set_wobble_mode(0, 0, 0, WobbleType::WT_DISABLE);
            }
        }
    }
    if (is_handling_high_speed_ && is_move_command) {
        is_handling_high_speed_ = false;
        // Note: GCodeGenerator moveTo add a small epsilon (0.001 < step) on border
        // Fix start position to the nearest step
        double start_pos = round(x_pos_ / high_speed_step_) * high_speed_step_;
        double current_pos = x_pos_;
        // Note: is_absolute_positioning should be false according to ToolpathExporter
        double final_pos = is_absolute_positioning ? x : x + x_pos_;
        bool is_reverse = final_pos < current_pos;
        double step = is_reverse ? -high_speed_step_ : high_speed_step_;
        int step_count = 0;
        int laser_power = settings.current_s;
        bool laser = false;
        bool completed = false;
        double new_x, x_move;
        std::bitset<4> bits;
        for (auto& c : high_speed_data_) {
            bits = c.isDigit() ? c.unicode() - '0' : c.unicode() - 'A' + 10;
            for (int i = 3; i >= 0; i--) {
                if (high_speed_data_count_ == step_count) {
                    completed = true;
                    break;
                }
                if (step_count == 0) {
                    // No need to handle laser off before the first pixel
                    step_count++;
                    laser = bits[i];
                    continue;
                }
                // Skip consecutive laser off
                if (laser || bits[i]) {
                    new_x = start_pos + step * step_count;
                    if (is_reverse != (new_x > final_pos)) {
                        // Limit new_x according to final position
                        new_x = final_pos;
                    }
                    if (is_absolute_positioning) {
                        x_move = round(new_x * 1000) / 1000;
                        current_pos = x_move;
                    } else {
                        x_move = round((new_x - current_pos) * 1000) / 1000;
                        current_pos += x_move;
                    }
                    handleGcode(QString("X%1S%2").arg(x_move).arg(laser ? laser_power : 0));
                }
                laser = bits[i];
                step_count++;
            }
            if (completed || !is_running_laser_) break;
        }
        if (std::fabs(final_pos - current_pos) > 0.001) {
            x_move = round((is_absolute_positioning ? final_pos: final_pos - current_pos) * 1000) /1000;
            handleGcode(QString("X%1S0").arg(x_move));
        }
        return;
    }

    if (command == "G90") {
      is_absolute_positioning = true;
    } else if (command == "G91") {
      is_absolute_positioning = false;
    } else if (command == "M3" || command == "M4") {
      // Begin Laser Control
      // qInfo() << "BSLM~::handleGcode() - M3M4: Laser Session Started" << getDebugTime();
      if (is_running_laser_) {
          qInfo() << "BSLM~::handleGcode() - M3M4: Laser already started" << getDebugTime();
          this->buffer_size_++; // Note: I guess this can be removed?
          return;
      }
      if (disconnect_count_ == -1) disconnect_count_ = 0;
      is_running_laser_ = true;
      // Reset current settings
      settings.current_s = 0;
      settings.current_f = 100.0;
      settings.period = 10;
      settings.pulse_width = 100;
      if (settings.wobble_diameter != -1) {
        settings.wobble_diameter = 0;
        settings.wobble_step = 0;
      }
      list_no = 1;

      // Dump all lcs status
      getListStatus();
      if (list_status.bMainOpen || list_status.bSubOepn || list_status.bCharOpen || list_status.bBusy1 || list_status.bBusy2 || list_status.bPaused || list_status.bLoop) {
        qInfo() << "BSLM~::handleGcode() - Irregular Status: " << list_status.bMainOpen << list_status.bSubOepn << list_status.bCharOpen << list_status.bLoop << list_status.bPaused << list_status.bBusy1 << list_status.bBusy2 << "@" << getDebugTime();
        if (list_status.bPaused) {
          doLcsApi(lcs_restart_list(),"lcs_restart_list");
        }
      }
      // Control instruction
      doLcsApi(lcs_set_jump_speed_ctrl(JUMP_SPEED),"lcs_set_jump_speed_ctrl");
      doLcsApi(lcs_set_mark_speed_ctrl(1000),"lcs_set_mark_speed_ctrl");
      doLcsApi(lcs_set_delay_mode(true, JUMP_DELAY_MIN, JUMP_DELAY_MAX, 10),"lcs_set_delay_mode");
      doLcsApi(lcs_set_laser_mode(LCS_MOPA, is_framing_),"lcs_set_laser_mode");
      startList(list_no, settings, true);
      // List Instruction
      doLcsApi(lcs_set_laser_delays(LASER_ON_DELAY, LASER_OFF_DELAY),"lcs_set_laser_delays");
      doLcsApi(lcs_set_scanner_delays(100, 50),"lcs_set_scanner_delays");
      lcs_error_count = 0;
      laser_enabled = false;
      before_first_laser = !is_framing_;
      should_swap = false;
      should_end = false;
      should_flush_ = false;
    } else if (command == "M2") {
      // qInfo() << "BSLM~::handleGcode() - M2: Ending Laser Control" << getDebugTime();
      if (last_is_z_command) {
        // Appand a dummy move command to ensure the last Z command is executed
        doLcsApi(lcs_set_axis_move(1, 1, z > 0, Z_SPEED_IN_PULSE, 10.0, 255),"lcs_set_axis_move");
      }
      doLcsApi(lcs_disable_laser(),"lcs_disable_laser");
      should_swap = true;
      should_end = true;
      rotary_mode = false;
    } else if (command == "M5") {
      qInfo() << "Turn Off Laser";
    } else if (command == "M99" ) {
      char sn[50];
      doLcsApi(lcs_get_serial_number(sn, 32),"lcs_get_serial_number");
      qInfo() << "BSLM~::handleGcode() - Serial Number: " << sn;
      if (sn[0] != '\0') {
        Q_EMIT configUpdate("serial", sn);
      }
    } else if (command == "M100") { 
      rotary_mode = false;
      qInfo() << "SetConfig: Rotary false";
    } else if (command == "M101") {
      rotary_mode = true;
      qInfo() << "SetConfig: Rotary true";
      doLcsApi(lcs_write_io_port(0b0),"lcs_write_io_port(0b0)");
    } else if (command == "M102") {
      qInfo() << "Enable OUT1/OUT2"; // Required for moving Z axis
      doLcsApi(lcs_write_io_port(0b0010),"lcs_write_io_port(0b0010)");
    } else if (command == "M103") {
      is_framing_ = true;
      qInfo() << "SetConfig: Framing true";
    } else if (command == "M104") {
      is_framing_ = false;
      qInfo() << "SetConfig: Framing false";
    } else if (command == "M105") {
      // Force reset position
      // Control instruction
      doLcsApi(lcs_goto_xy(0, 0),"lcs_goto_xy(0, 0)");
    } else if (!is_move_command) {
      return;
    }

    this->buffer_size_++;
    
    if (this->buffer_size_ >= MAX_BUFFER_LIST_SIZE) {
        should_swap = true;
    }

    if (should_swap || is_running_laser_ && should_flush_) {
      should_flush_ = should_swap = false;
      qInfo() << "BSLM~::handleGcode() - Flushing buffer with size" << this->buffer_size_ << "@" << getDebugTime();
      qInfo() << "BSLM~::handleGcode() - Executing list" << list_no << "@" << getDebugTime();
      if(!executeList(list_no)) return;
      list_no = list_no == 1 ? 2 : 1;
      waitListAvailable(list_no);
      startList(list_no, settings, should_end);
      qInfo("BSLM~::handleGcode() - Swap new list %d", list_no);
      this->buffer_size_ = 0;
      QThread::msleep(1);
    }

    if (should_end) {
      // qInfo() << "BSLM~::handleGcode() - Ending Laser Control"  << "@" << getDebugTime();
      // qInfo() << "BSLM~::handleGcode() - Executing list" << list_no << "@" << getDebugTime();
      if(!executeList(list_no)) return;
      QThread::msleep(2);
      list_no = list_no == 1 ? 2 : 1;
      waitListAvailable(list_no); // Wait till the previous list is available.
      doLcsApi(lcs_set_end_of_list(),"lcs_set_end_of_list"); // Send empty list
      if(!executeList(list_no)) return;
      QThread::msleep(1);
      list_no = list_no == 1 ? 2 : 1;
      waitListAvailable(list_no); // Wait till the previous list is available.
      doLcsApi(lcs_set_end_of_list(),"lcs_set_end_of_list"); // Send empty list
      if(!executeList(list_no)) return;
      QThread::msleep(1);

      BoardRunStatus Status;
      do {
          uint32_t Pos;
          LCS2Error ret = doLcsApi(lcs_get_status((uint32_t *)&Status, &Pos),"lcs_get_status");

          if (ret != LCS_RES_NO_ERROR)
              break;
      } while (!Status.bCacheReady);
      // qInfo() << "BSLM~::handleGcode() - [Laser Session Closed]" << "@" << getDebugTime();
      // qInfo() << "BSLM~::handleGcode() - Pending commands: " << this->pending_cmds_.size();
      is_running_laser_ = false;
      laser_enabled = false;
      should_end = false;
      if (!is_framing_) {
        doLcsApi(lcs_set_laser_control(false),"lcs_set_laser_control(false)");
        doLcsApi(lcs_goto_xy(0, 0),"lcs_goto_xy(0, 0)");
      }
    }

    // Process move command

    if (z != 0) {
      qInfo() << "BSLM~::handleGcode() - Z Axis" << z;
      doLcsApi(lcs_set_axis_move(1, fabs(z) * Z_PULSE_PER_MM, z > 0, Z_SPEED_IN_PULSE, 10, 255),"lcs_set_axis_move 1");
      estimated_time_ += fabs(z) * Z_SPEED_IN_MM * 1000;
      last_is_z_command = true;
      // QThread::msleep(1000);
    } else if (is_move_command) {
      double target_x, target_y;
      if (is_absolute_positioning) {
          target_x = x;
          target_y = y;
      } else {
          target_x = x_pos_ + x;
          target_y = y_pos_ + y;
      }

      if (rotary_mode) {
        double diff_y = target_y - y_pos_;
        if (diff_y != 0) {
          doLcsApi(lcs_set_axis_move(0, fabs(diff_y) * 100, diff_y < 0, 3200, 1600, 255),"lcs_set_axis_move 0");
          // TODO: calulate estimated time
        }
        double distance = fabs(target_x - x_pos_);
        if (laser_enabled && (command == "G1" || command.isEmpty())) {
            if (dotting_time == 0) {
                mark_to(0, target_x - center_pos);
                estimated_time_ += (distance * wobble_k) / settings.current_f * 1000 + LASER_DELAY;
            } else {
                jump_to(0, target_x - center_pos);
                estimated_time_ += distance / JUMP_SPEED * 1000 + JUMP_DELAY;
                doLcsApi(lcs_laser_on_list(dotting_time),"lcs_laser_on_list");
                estimated_time_ += dotting_time / 1000;
            }
        } else {
            jump_to(0, target_x - center_pos);
            estimated_time_ += distance / JUMP_SPEED * 1000 + JUMP_DELAY;
        }
      } else {
        double distance = sqrt(pow(target_x - x_pos_, 2) + pow(target_y - y_pos_, 2));
        if (laser_enabled && (command == "G1" || command.isEmpty())) {
            if (dotting_time == 0) {
                mark_to(-(target_y - center_pos), target_x - center_pos);
                estimated_time_ += (distance * wobble_k) / settings.current_f * 1000 + LASER_DELAY;
            } else {
                jump_to(-(target_y - center_pos), target_x - center_pos);
                estimated_time_ += distance / JUMP_SPEED * 1000 + JUMP_DELAY;
                doLcsApi(lcs_laser_on_list(dotting_time),"lcs_laser_on_list");
                estimated_time_ += dotting_time / 1000;
            }
        } else {
            jump_to(-(target_y - center_pos), target_x - center_pos);
            estimated_time_ += distance / JUMP_SPEED * 1000 + JUMP_DELAY;
        }
      }
      x_pos_ = target_x;
      y_pos_ = target_y;
      last_is_z_command = false;
    }
}

/**
 * @brief Send the cmd_packet immediately
 *        
 * @param executor cmd sender (cmd source)
 * @param cmd_packet 
 * @return true if cmd_packet is sent
 * @return false if port is busy or buffer is full
 */
MotionController::CmdSendResult BSLMotionController::sendCmdPacket(QPointer<Executor> executor, QString cmd_packet) {
  qInfo() << "SetLock: try lock() in sendCmdPacket";
  this->cmd_list_mutex_.lock();
  qInfo() << "SetLock: lock() in sendCmdPacket";
  this->pending_cmds_.push(cmd_packet);
  enqueueCmdExecutor(executor);
  this->cmd_list_mutex_.unlock();
  qInfo() << "SetLock: unlock() in sendCmdPacket";
  if (!this->command_runner_thread_.joinable()) {
    startCommandRunner();
  }
  return CmdSendResult::kOk;
}

/**
 * @brief Handle response from GRBL board
 * 
 * @param resp 
 */
void BSLMotionController::respReceived(QString resp) {
  qWarning() << "BSLM~::respReceived is not supported.";
  return;
}

// TODO:BSL
void BSLMotionController::attachPortBSL() {
  qInfo() << "MotionController::attachPortBSL()";
  // Actually do nothing..
  setState(MotionControllerState::kIdle);
}

MotionController::CmdSendResult BSLMotionController::pause() {
  if (std::this_thread::get_id() == command_runner_thread_.get_id()) {
    throw std::runtime_error("BSLM~::pause() - This function should not be called within the command runner thread");
  }
  setState(MotionControllerState::kPaused);
  return CmdSendResult::kOk;
}

MotionController::CmdSendResult BSLMotionController::resume() {
  if (std::this_thread::get_id() == command_runner_thread_.get_id()) {
    throw std::runtime_error("BSLM~::pause() - This function should not be called within the command runner thread");
  }
  setState(MotionControllerState::kRun);
  return CmdSendResult::kOk;
}

MotionController::CmdSendResult BSLMotionController::stop() {
  qInfo() << "BSLM~::stop() @" << getDebugTime();
  if (this->current_error_) {
    this->setState(MotionControllerState::kAlarm);
  } else {
    this->setState(MotionControllerState::kSleep);
  }
  qInfo() << "BSLM~::stop() - Clearing pending commands" << getDebugTime();
  doLcsApi(lcs_set_end_of_list(),"lcs_set_end_of_list");
  doLcsApi(lcs_stop_execution(),"lcs_stop_execution");
  running_task_time_ = 0;
  this->is_running_laser_ = false;
  getBoardStatus();
  if (!status.bConnected) lcs_connect();
  qInfo() << "SetLock: try lock() in stop";
  this->cmd_list_mutex_.lock();
  qInfo() << "SetLock: lock() in stop";
  std::queue<QString> new_queue;
  this->pending_cmds_.swap(new_queue);
  this->cmd_list_mutex_.unlock();
  qInfo() << "SetLock: unlock() in stop";
  dequeueCmd(this->cmd_executor_queue_.size());
  Q_EMIT MotionController::resetDetected();
  QThread::msleep(200);
  handleGcode("M105");
  return CmdSendResult::kOk;
}

bool BSLMotionController::detachPort() {
  qInfo() << "BSLM~::detachPort()";
  if (this->command_runner_thread_.joinable()) {
    setState(MotionControllerState::kQuit);
    command_runner_thread_.join();
  }
  stop();
  LCS2Error result = doLcsApi(lcs_release_card(0),"lcs_release_card");
  Q_EMIT disconnected();
  setState(MotionControllerState::kQuit);
  qInfo() << "BSLM~::detachPort() finished";
  return result == LCS_RES_NO_ERROR;
}

bool BSLMotionController::resetState() {
  qInfo() << "BSLM~::resetState()" << getDebugTime();
  this->current_error_ = 0;
  this->disconnect_count_ = -1;
  switch (getState()) {
    case MotionControllerState::kIdle:
      return true;
    case MotionControllerState::kRun:
    case MotionControllerState::kPaused:
      this->stop();
      dequeueCmd(this->cmd_executor_queue_.size());
      this->setState(MotionControllerState::kIdle);
      return true;
    case MotionControllerState::kAlarm:
      this->setState(MotionControllerState::kIdle);
      return true;
    case MotionControllerState::kSleep:
      this->setState(MotionControllerState::kIdle);
      return true;
    case MotionControllerState::kUnknown:
    case MotionControllerState::kQuit:
      return false;
  }
}

void BSLMotionController::setCorrection(double scaleX, double scaleY,double bucketX,double bucketY,double paralleX,double paralleY,double trapeX,double trapeY) {
  LCS2Error ret = lcs_set_manual_correction_params(scaleX, scaleY, bucketX, bucketY, paralleX, paralleY, trapeX, trapeY);
  qInfo() << "BSLM~::setCorrection() - Correction set result = " << getErrorString(ret);
}

void BSLMotionController::setScanaheadParams(double worksize, double angle, double xOffset, double yOffset) {
  LCS2Error ret = lcs_set_scanahead_params(worksize, false, false, false, angle, xOffset, yOffset);
  qInfo() << "BSLM~::setScanaheadParams() - Scanahead Params set result = " << getErrorString(ret);
}

std::mutex state_mutex_;
BoardRunStatus BSLMotionController::getBoardStatus() {
  qInfo() << "SetLock: try lock() [state] in getBoardStatus";
  std::lock_guard<std::mutex> lock(state_mutex_);
  qInfo() << "SetLock: lock() [state] in getBoardStatus";
  status.bConnected = false;
  LCS2Error ret = lcs_get_status((uint32_t *)&status, &pos);
  qInfo() << "SetLock: auto release [state] in getBoardStatus";
  if (ret == LCS_RES_NO_ERROR) return status;
  return status;
}

ListStatus BSLMotionController::getListStatus() {
  lcs_read_status((uint32_t *)&list_status);
  return list_status;
}

std::mutex reconnect_mutex_;
bool BSLMotionController::isConnected() {
  if (is_board_connected_ != getBoardStatus().bConnected) {
    is_board_connected_ = !is_board_connected_;
    if (!is_board_connected_) {
      qInfo() << "BSLM~::isConnected() - Board disconnected, running:" << is_running_laser_ << "framing:" << is_framing_;
      if (!is_running_laser_) {
        // Handle running thread first
        QThread::msleep(1000);
      }
      qInfo() << "SetLock: try lock() [reconnect] in isConnected";
      std::lock_guard<std::mutex> lock(reconnect_mutex_);
      qInfo() << "SetLock: lock() [reconnect] in isConnected";
      is_board_connected_ = getBoardStatus().bConnected;
      if(!is_board_connected_){
        // Stop execution to avoid lcs crash
        doLcsApi(lcs_pause_list(),"lcs_pause_list");
        doLcsApi(lcs_release_card(0),"lcs_release_card");
        this->disconnect_count_++;
        getListStatus();
        if (is_running_laser_ && !is_framing_ && running_task_time_ > 0 && task_timer_.isValid()) {
          // Note: Current list will be abort when lcs_assign_card
          // Wait for the current task to finish then reconnect
          // Add addtional 3s for starting time and tolerance
          int remaining_time = running_task_time_ - task_timer_.elapsed() + 3000;
          if (remaining_time > 0) QThread::msleep(remaining_time);
        }
        for (int i = 0; i < 3 && !is_board_connected_; i++) {
          QThread::msleep(2000);
          qInfo() << "Try reconnecting to the board" << i;
          is_board_connected_ = lcs_connect();
        }
        qInfo() << "Try reconnecting to the board - done" << is_board_connected_;
        if (!is_board_connected_) {
          stop();
          Q_EMIT disconnected();
        } else if (getState() == MotionControllerState::kRun) {
          doLcsApi(lcs_restart_list(),"lcs_restart_list");
        }
      }
      qInfo() << "SetLock: auto release [reconnect] in isConnected";
    }
  }
  return is_board_connected_;
}

void BSLMotionController::startList(int list_no, TaskSettings settings, bool disable_laser) {
  estimated_time_ = 0;
  doLcsApi(lcs_set_start_list(list_no),"lcs_set_start_list" + QString::number(list_no));
  // Reset laser control in case of disconnection
  if (is_framing_ || disable_laser) {
    doLcsApi(lcs_disable_laser(),"lcs_disable_laser");
  } else {
    doLcsApi(lcs_set_laser_control(true),"lcs_set_laser_control true");
    doLcsApi(lcs_enable_laser(),"lcs_enable_laser");
  }
  doLcsApi(lcs_set_laser_pulses(settings.period, 0, settings.pulse_width),"lcs_set_laser_pulses");
  doLcsApi(lcs_set_mark_speed(settings.current_f),"lcs_set_mark_speed");
  doLcsApi(lcs_set_laser_power(settings.current_s / 10),"lcs_set_laser_power");
  if (settings.wobble_step > 0 && settings.wobble_diameter > 0) {
    doLcsApi(lcs_set_wobble_mode(settings.wobble_diameter, settings.wobble_diameter, settings.wobble_step, WobbleType::WT_WHEEL),"lcs_set_wobble_mode");
  } else if (settings.wobble_diameter != -1) {
    doLcsApi(lcs_set_wobble_mode(0, 0, 0, WobbleType::WT_DISABLE),"lcs_set_wobble_mode");
  }
}

bool BSLMotionController::executeList(int list_no) {
  doLcsApi(lcs_set_end_of_list(),"lcs_set_end_of_list");
  if(!is_framing_){
    // Wait for last list completion
    double max_waiting_time = running_task_time_ + 3000;
    if (!task_timer_.isValid()) task_timer_.start();
    do {
      QThread::msleep(100);
      // Update status and trigger reconnect if disconnected
      isConnected();
      getListStatus();
      if (task_timer_.elapsed() > max_waiting_time) {
        // In case bBusy1 and bBusy2 are not updated
        qInfo() << "BSLM~::executeList() - Timeout waiting for list completion" << getDebugTime();
        break;
      }
    } while (is_running_laser_ && running_task_time_ > 0 &&
            (list_status.bPaused || list_status.bBusy1 || list_status.bBusy2));
  }
  if (!is_running_laser_ || !status.bConnected) return false;
  int e = doLcsApi(lcs_execute_list(list_no),"lcs_execute_list" + QString::number(list_no));
  if (e != LCS_RES_NO_ERROR) {
    if (e == LCS_GENERAL_CURRENTLY_BUSY) {
      // Sometimes happens after reconnecting
      // Board is connected but not able to execute list
      qInfo() << "Board connected but currently busy; force reconnecting" << getDebugTime();
      lcs_connect(true);
    }
    // Trigger reconnect
    bool is_connected = isConnected();
    e = doLcsApi(lcs_execute_list(list_no),"lcs_execute_list" + QString::number(list_no));
    if (e != LCS_RES_NO_ERROR) {
      qInfo() << "BSLM~::executeList() - Error executing list" << getErrorString(e);
      this->current_error_ = e;
      this->stop();
      return false;
    }
  }
  if (task_timer_.isValid()) task_timer_.restart();
  else task_timer_.start();
  running_task_time_ = estimated_time_;
  estimated_time_ = 0;
  return true;
}
