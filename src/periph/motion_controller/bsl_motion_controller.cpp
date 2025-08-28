#include "bsl_motion_controller.h"
#include "constants.h"

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
#define MAX_BUFFER_LIST_TIME 30000

int lcs_error_count = 0;
uint32_t pos;
BoardRunStatus status;
ListStatus list_status;

QString BSLMotionController::getErrorString(int error) {
  switch (error) {
    case LCS_RES_NO_ERROR:
      return "LCS_RES_NO_ERROR";
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

void BSLMotionController::checkPauseResume() {
  isConnected();
  QCoreApplication::processEvents();
  bool should_do_pause = false;
  bool should_do_resume = false;
  switch (this->getState()) {
    case MotionControllerState::kPaused:
      if (!lcs_paused_) {
        // First loop after pause
        should_do_pause = true;
      }
      break;
    case MotionControllerState::kIdle:
    case MotionControllerState::kRun:
      if (lcs_paused_) {
        // First loop after resume
        should_do_resume = true;
      }
  }
  if (should_check_door_ && is_running_laser_ && !is_framing_ && (should_do_resume || !lcs_paused_)) {
    // Check door status
    uint32_t io_port = lcs_read_io_port();
    if (io_port & 0b1) {
      qInfo() << "BSLM~::thread() - door opened, pause task";
      should_do_pause = true;
      this->setState(MotionControllerState::kPaused);
      current_custom_error_ = "HARDWARE_ERROR,DOOR_OPENED";
    }
  }
  if (should_do_pause) {
    lcs_pause_list();
    pauseTimer();
    lcs_paused_ = true;
    qInfo() << "BSLM~::thread() - pausing";
  } else if (should_do_resume) {
    lcs_restart_list();
    resetTimer();
    lcs_paused_ = false;
    QThread::msleep(25);
    qInfo() << "BSLM~::thread() - resuming";
  }
}

void BSLMotionController::commandRunnerThread() {
  qInfo() << "BSLM~::thread() - entered @" << getDebugTime() << " - pending cmds.." << pending_cmds_.size();
  while (this->getState() != MotionControllerState::kQuit) {
    checkPauseResume();
    debug_count_bsl ++;
    switch (this->getState()) {
      case MotionControllerState::kPaused:
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
        if (debug_count_bsl % 40 == 1) qInfo() << "BSLM~::thread() - Alarm State" << getCurrentError();
        QThread::msleep(25); 
        break;
      case MotionControllerState::kIdle:
      case MotionControllerState::kRun:
        this->cmd_list_mutex_.lock();
        if (this->pending_cmds_.empty()) {
          this->cmd_list_mutex_.unlock();
          if (debug_count_bsl % 40 == 1) {
            qInfo() << "BSLM~::thread() - No pending commands, wait 1s. Board Connection: " << isConnected() << "@" << getDebugTime();
            if (!isConnected()) {
              this->setState(MotionControllerState::kQuit); // Invalid this motion controller once the connection is lost
              break;
            }
          }
          setState(MotionControllerState::kIdle); // Set state to idle if there are no pending commands
          QThread::msleep(25);
        } else {
          this->cmd_list_mutex_.unlock();
          if (debug_count_bsl % 1000 == 1) {
            qInfo() << "BSLM~::thread() - pending commands: " << this->pending_cmds_.size();
          }
          bool is_connected = isConnected();
          if (!is_connected) {
            this->setState(MotionControllerState::kQuit);
            break;
          }
          this->cmd_list_mutex_.lock();
          QString cmd = this->pending_cmds_.front();
          this->pending_cmds_.pop();
          this->cmd_list_mutex_.unlock();
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
  this->cmd_list_mutex_.lock();
  for (int i = 0; i < count; i++) {
    if (!cmd_executor_queue_.isEmpty()) {
      auto exec = cmd_executor_queue_.at(0);
      exec->handleCmdFinish(0);
      dequeueCmdExecutor();
    }
  }
  this->cmd_list_mutex_.unlock();
}

LCS2Error BSLMotionController::waitListAvailable(int list_no) {
  qInfo() << "BSLM~::waitList(" << list_no << ")@" << getDebugTime();
  LCS2Error ret = lcs_load_list(list_no, 0);
  bool fixing_aready = false;
  while (ret != LCS_RES_NO_ERROR) {
    QThread::msleep(25);
    checkPauseResume();
    if (lcs_paused_) {
      qInfo() << "BSLM~::waitList - Paused while waitListAvailable!";
      continue;
    }
    qInfo() << getErrorString(ret);
    ret = lcs_load_list(list_no, 0);
    // If the list is already opened, close the list
    if (ret == LCS_GENERAL_AREADY_OPENED) {
      qInfo() << "BSLM~::waitListAvailable(" << list_no << ") - List already opened" << getDebugTime();
      if (!fixing_aready) {
        fixing_aready = true;
        lcs_set_start_list(list_no);
        lcs_set_end_of_list();
      }
      ret = lcs_load_list(list_no, 0);
    }
    if (lcs_error_count ++ > 100) {
      qWarning() << "BSLM~::waitListAvailable(" << list_no << ") - Error count exceeded 100" << getDebugTime();
      this->current_error_ = ret;
      this->current_custom_error_ = "Failed to load list";
      this->stop();
      break;
    }
  }
  return ret;
}

void jump_to(double y, double x) {
  // If xy is inverted swap x, y
  lcs_jump_abs(y, x);
}

void mark_to(double y, double x) {
  // If xy is inverted, swap x, y
  lcs_mark_abs(y, x);
}

void BSLMotionController::handleGcode(const QString &gcode) {
    static TaskSettings settings;
    static bool laser_enabled = false;
    static bool is_absolute_positioning = true;
    static int list_no = 1;
    static double center_pos = 55;
    static int freq = 100; //100 khz
    static bool last_is_z_command = false;
    static int dotting_time = 0;
    static bool before_first_laser = true;
    static double wobble_k = 1;
    static QRegularExpression re("([GMXYFSZDWQPTA]|WD|WS)(-?\\d+\\.?\\d*)");
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
    double target_a = a_pos_;
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
        } else if (type == "A") {
            target_a = value.toDouble();
            is_move_command = true;
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
            lcs_set_laser_pulses(settings.period, 0, settings.pulse_width);
        } else if (type == "P") {
            settings.pulse_width = value.toInt();
            lcs_set_laser_pulses(settings.period, 0, settings.pulse_width);
        } else if (type == "T") {
            dotting_time = value.toInt();
        } else if (type == "F") {
            settings.current_f = value.toDouble() / 60;
            lcs_set_mark_speed(settings.current_f);
        } else if (type == "S") {
            settings.current_s = value.toInt();
            if (!is_handling_high_speed_) {
                if (settings.current_s > 0) {
                  laser_enabled = true;
                  lcs_set_laser_power(settings.current_s / 10);  // Assuming S1000 is 100% power
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
            checkPauseResume();
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
      current_error_ = 0;
      current_custom_error_.clear();
      // Reset current settings
      dotting_time = 0;
      a_pos_ = 0;
      settings.current_s = 0;
      settings.current_f = 100.0;
      settings.period = 10;
      settings.pulse_width = 100;
      if (settings.wobble_diameter != -1) {
        settings.wobble_diameter = 0;
        settings.wobble_step = 0;
        wobble_k = 1;
      }
      list_no = 1;

      // Dump all lcs status
      getListStatus();
      if (list_status.bMainOpen || list_status.bSubOepn || list_status.bCharOpen || list_status.bBusy1 || list_status.bBusy2 || list_status.bPaused || list_status.bLoop) {
        qInfo() << "BSLM~::handleGcode() - Irregular Status: " << list_status.bMainOpen << list_status.bSubOepn << list_status.bCharOpen << list_status.bLoop << list_status.bPaused << list_status.bBusy1 << list_status.bBusy2 << "@" << getDebugTime();
        if (list_status.bMainOpen) {
          lcs_stop_execution();
          lcs_set_start_list(1);
          lcs_set_end_of_list();
          lcs_set_start_list(2);
          lcs_set_end_of_list();
        }
        if (list_status.bPaused) lcs_restart_list();
      }
      // Control instruction
      lcs_set_jump_speed_ctrl(PromarkJobConfig::JUMP_SPEED);
      lcs_set_mark_speed_ctrl(1000);
      lcs_set_delay_mode(true, PromarkJobConfig::JUMP_DELAY_MIN, PromarkJobConfig::JUMP_DELAY_MAX, 10);
      lcs_set_laser_mode(LCS_MOPA, is_framing_);
      startList(list_no, settings, true);
      // List Instruction
      // Force delay for the first laser
      lcs_set_laser_delays(-3000, PromarkJobConfig::LASER_OFF_DELAY);
      lcs_set_scanner_delays(100, 50);
      lcs_error_count = 0;
      laser_enabled = false;
      last_is_z_command = false;
      before_first_laser = !is_framing_;
      should_swap = false;
      should_end = false;
      should_flush_ = false;
    } else if (command == "M2") {
      // qInfo() << "BSLM~::handleGcode() - M2: Ending Laser Control" << getDebugTime();
      if (last_is_z_command) {
        // Appand a dummy move command to ensure the last Z command is executed
        lcs_set_axis_move(1, 1, z > 0, PromarkJobConfig::Z_PULSE_PER_SEC, 10.0, 255);
      }
      lcs_disable_laser();
      should_swap = true;
      should_end = true;
      settings.rotary_mode = false;
      lcs_write_io_port_mask_list(0b01, 0b11);
    } else if (command == "M5") {
      qInfo() << "Turn Off Laser";
    } else if (command == "M99" ) {
      char sn[50];
      lcs_get_serial_number(sn, 32);
      qInfo() << "BSLM~::handleGcode() - Serial Number: " << sn;
      if (sn[0] != '\0') {
        Q_EMIT configUpdate("serial", sn);
      }
    } else if (command == "M100") {
      // Rotary io: 1st port, 1 -> off, 0 -> on
      settings.rotary_mode = false;
      lcs_write_io_port_mask(0b1, 0b1);  // Control instruction
    } else if (command == "M101") {
      settings.rotary_mode = true;
      lcs_write_io_port_mask_list(0b0, 0b1);
    } else if (command == "M102") {
      // Z axis io: 2nd port, 1 -> on, 0 -> off
      lcs_write_io_port_mask_list(0b10, 0b10);
    } else if (command == "M103") {
      is_framing_ = true;
      is_running_laser_ = false;
    } else if (command == "M104") {
      is_framing_ = false;
      is_running_laser_ = false;
    } else if (command == "M105") {
      // Control instruction
      // Force reset position
      lcs_goto_xy(0, 0);
      // Loose motor
      lcs_write_io_port_mask(0b01, 0b11);
    } else if (!is_move_command) {
      return;
    }

    this->buffer_size_++;

    if (this->buffer_size_ >= MAX_BUFFER_LIST_SIZE || estimated_time_ >= MAX_BUFFER_LIST_TIME) {
        should_swap = true;
    }

    if (should_swap || is_running_laser_ && should_flush_) {
      should_flush_ = should_swap = false;
      qInfo() << "BSLM~::handleGcode() - Flushing buffer with size" << this->buffer_size_ << "and time" << running_task_time_ << "@" << getDebugTime();
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
      if(!executeList(list_no)) return;
      QThread::msleep(1);
      list_no = list_no == 1 ? 2 : 1;
      waitListAvailable(list_no); // Wait till the previous list is available.
      if(!executeList(list_no)) return;
      QThread::msleep(1);

      BoardRunStatus Status;
      do {
          uint32_t Pos;
          LCS2Error ret = lcs_get_status((uint32_t *)&Status, &Pos);

          if (ret != LCS_RES_NO_ERROR)
              break;
      } while (!Status.bCacheReady);
      // qInfo() << "BSLM~::handleGcode() - [Laser Session Closed]" << "@" << getDebugTime();
      // qInfo() << "BSLM~::handleGcode() - Pending commands: " << this->pending_cmds_.size();
      is_running_laser_ = false;
      laser_enabled = false;
      should_end = false;
      if (!is_framing_) {
        lcs_set_laser_control(false);
        lcs_goto_xy(0, 0);
      }
    }

    // Process move command

    if (z != 0) {
      qInfo() << "BSLM~::handleGcode() - Z Axis" << z;
      lcs_set_axis_move(1, fabs(z) * PromarkJobConfig::Z_PULSE_PER_MM, z > 0, PromarkJobConfig::Z_PULSE_PER_SEC, 10, 255);
      estimated_time_ += fabs(z) * PromarkJobConfig::Z_MS_PER_MM;
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

      double diff_a = target_a - a_pos_;
      if (diff_a != 0) {
        double real_steps = round(diff_a * PromarkJobConfig::A_PULSE_PER_MM);
        lcs_set_axis_move(0, fabs(real_steps), diff_a > 0, PromarkJobConfig::A_PULSE_PER_SEC, 1600, 255);
        estimated_time_ += fabs(diff_a) * PromarkJobConfig::A_MS_PER_MM;
        a_pos_ += real_steps / PromarkJobConfig::A_PULSE_PER_MM;
      }
      double distance = sqrt(pow(target_x - x_pos_, 2) + pow(target_y - y_pos_, 2));
      if (distance > 0) {
        if (laser_enabled && (command == "G1" || command.isEmpty())) {
          if (dotting_time == 0) {
            mark_to(-(target_y - center_pos), target_x - center_pos);
            estimated_time_ += (distance * wobble_k) / settings.current_f * 1000 + PromarkJobConfig::LASER_DELAY_MS;
          } else {
            jump_to(-(target_y - center_pos), target_x - center_pos);
            estimated_time_ += distance / PromarkJobConfig::JUMP_SPEED * 1000 + PromarkJobConfig::JUMP_DELAY_MS;
            lcs_laser_on_list(dotting_time);
            estimated_time_ += dotting_time / 1000.0;
          }
        } else {
          jump_to(-(target_y - center_pos), target_x - center_pos);
          estimated_time_ += distance / PromarkJobConfig::JUMP_SPEED * 1000 + PromarkJobConfig::JUMP_DELAY_MS;
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
  this->cmd_list_mutex_.lock();
  this->pending_cmds_.push(cmd_packet);
  enqueueCmdExecutor(executor);
  this->cmd_list_mutex_.unlock();
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
  setState(lcs_connect() ? MotionControllerState::kIdle : MotionControllerState::kQuit);
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
  // Clear door error
  current_custom_error_.clear();
  setState(MotionControllerState::kRun);
  return CmdSendResult::kOk;
}

MotionController::CmdSendResult BSLMotionController::stop() {
  qInfo() << "BSLM~::stop() @" << getDebugTime();
  if (!getCurrentError().isNull()) {
    this->setState(MotionControllerState::kAlarm);
  } else {
    this->setState(MotionControllerState::kSleep);
  }
  qInfo() << "BSLM~::stop() - Clearing pending commands" << getDebugTime();
  lcs_set_end_of_list();
  lcs_stop_execution();
  running_task_time_ = 0;
  this->is_running_laser_ = false;
  getBoardStatus();
  if (!status.bConnected) lcs_connect();
  this->cmd_list_mutex_.lock();
  std::queue<QString> new_queue;
  this->pending_cmds_.swap(new_queue);
  this->cmd_list_mutex_.unlock();
  dequeueCmd(this->cmd_executor_queue_.size());
  Q_EMIT MotionController::resetDetected();
  QThread::msleep(200);
  // Restore the state in case it is paused
  lcs_paused_ = false;
  lcs_restart_list();
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
  LCS2Error result = lcs_release_card(0);
  Q_EMIT disconnected();
  setState(MotionControllerState::kQuit);
  qInfo() << "BSLM~::detachPort() finished";
  return result == LCS_RES_NO_ERROR;
}

bool BSLMotionController::resetState() {
  qInfo() << "BSLM~::resetState()" << getDebugTime();
  this->current_error_ = 0;
  this->current_custom_error_.clear();
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
  std::lock_guard<std::mutex> lock(state_mutex_);
  status.bConnected = false;
  LCS2Error ret = lcs_get_status((uint32_t *)&status, &pos);
  if (ret == LCS_RES_NO_ERROR) return status;
  qInfo() << "BSLM~::getBoardStatus() - bConnected" << status.bConnected << "Error" << getErrorString(ret);
  return status;
}

ListStatus BSLMotionController::getListStatus() {
  lcs_read_status((uint32_t *)&list_status);
  return list_status;
}

std::mutex reconnect_mutex_;
bool BSLMotionController::isConnected() {
  is_board_connected_ = getBoardStatus().bConnected;
  if (!is_board_connected_) {
    qInfo() << "BSLM~::isConnected() - Board disconnected, running:" << is_running_laser_ << "framing:" << is_framing_;
    if (!is_running_laser_) {
      // Handle running thread first
      QThread::msleep(1000);
    }
    std::lock_guard<std::mutex> lock(reconnect_mutex_);
    is_board_connected_ = getBoardStatus().bConnected;
    if(!is_board_connected_){
      // Stop execution to avoid lcs crash
      is_handling_reconnection_ = true;
      lcs_pause_list();
      lcs_release_card(0);
      this->disconnect_count_++;
      getListStatus();
      if (is_running_laser_ && !lcs_paused_ && !is_framing_ && running_task_time_ > 0 && task_timer_.isValid()) {
        // Note: Current list will be aborted when lcs_assign_card
        // Wait for the current task to finish then reconnect
        int remaining_time = getRemainingTime();
        if (remaining_time > 0) {
          qInfo() << "BSLM~::isConnected() - Waiting for current task to finish" << remaining_time;
          QThread::msleep(remaining_time);
        }
      }
      for (int i = 0; i < 3 && !is_board_connected_; i++) {
        QThread::msleep(2000);
        qInfo() << "Try reconnecting to the board" << i;
        is_board_connected_ = lcs_connect();
      }
      is_handling_reconnection_ = false;
      qInfo() << "Try reconnecting to the board - done" << is_board_connected_;
      if (!is_board_connected_) {
        this->current_custom_error_ = "DISCONNECTED";
        stop();
        Q_EMIT disconnected();
      } else if (lcs_paused_) {
        stop();
      } else if (getState() == MotionControllerState::kRun) {
        lcs_restart_list();
      }
    }
  }
  return is_board_connected_;
}

void BSLMotionController::startList(int list_no, TaskSettings settings, bool disable_laser) {
  estimated_time_ = 0;
  lcs_set_start_list(list_no);
  // Reset laser control in case of disconnection
  lcs_set_laser_control(true);
  lcs_enable_laser();
  lcs_set_laser_pulses(settings.period, 0, settings.pulse_width);
  lcs_set_mark_speed(settings.current_f);
  lcs_set_laser_power(settings.current_s / 10);
  if (settings.wobble_step > 0 && settings.wobble_diameter > 0) {
    lcs_set_wobble_mode(settings.wobble_diameter, settings.wobble_diameter, settings.wobble_step, WobbleType::WT_WHEEL);
  } else if (settings.wobble_diameter != -1) {
    lcs_set_wobble_mode(0, 0, 0, WobbleType::WT_DISABLE);
  }
  lcs_set_laser_delays(PromarkJobConfig::LASER_ON_DELAY, PromarkJobConfig::LASER_OFF_DELAY);
  if (settings.rotary_mode) {
    lcs_write_io_port_mask_list(0b0, 0b1);
  }
}

bool BSLMotionController::executeList(int list_no) {
  qInfo() << "BSLM~::executeList(" << list_no << ") @" << getDebugTime();
  lcs_set_end_of_list();
  if(!is_framing_ && running_task_time_ > 0){
    // Wait for last list completion
    int count = 0;
    do {
      QThread::msleep(100);
      // Update status and trigger reconnect if disconnected
      isConnected();
      getListStatus();
      checkPauseResume();
      if (!lcs_paused_ && getRemainingTime() < 0) {
        // In case bBusy1 and bBusy2 are not updated
        qInfo() << "BSLM~::executeList() - Timeout waiting for list completion @" << getDebugTime();
        break;
      }
      if (++count % 10 == 0) {
        qInfo() << "BSLM~::executeList() - Waiting for previous list..." << "paused" << lcs_paused_ << "list paused" << list_status.bPaused << "busy1" << list_status.bBusy1 << "busy2" << list_status.bBusy2;
      }
    } while (is_running_laser_ && (lcs_paused_ || list_status.bPaused || list_status.bBusy1 || list_status.bBusy2));
  }
  if (!is_running_laser_ || !status.bConnected) return false;
  qInfo() << "BSLM~::executeList() - 1st try to execute list" << list_no << "@" << getDebugTime();
  int e = lcs_execute_list(list_no);
  qInfo() << "BSLM~::executeList() - Result of 1st try" << getErrorString(e) << "@" << getDebugTime();
  if (e != LCS_RES_NO_ERROR) {
    if (e == LCS_GENERAL_CURRENTLY_BUSY) {
      // Sometimes happens after reconnecting
      // Board is connected but not able to execute list
      qInfo() << "Board connected but currently busy; force reconnecting" << getDebugTime();
      lcs_connect(true);
    }
    // Trigger reconnect
    qInfo() << "BSLM~::executeList() - Check connection before 2nd try" << "@" << getDebugTime();
    bool is_connected = isConnected();
    qInfo() << "BSLM~::executeList() - 2nd try to execute list" << list_no << is_connected << "@" << getDebugTime();
    e = lcs_execute_list(list_no);
    qInfo() << "BSLM~::executeList() - Result of 2nd try" << getErrorString(e) << "@" << getDebugTime();
    if (e != LCS_RES_NO_ERROR) {
      qInfo() << "BSLM~::executeList() - Error executing list" << getErrorString(e);
      this->current_error_ = e;
      this->current_custom_error_ = "Failed to execute list";
      this->stop();
      return false;
    }
  }
  running_task_time_ = estimated_time_;
  qInfo() << "BSLM~::executeList() - Start executing new list, task time:" << running_task_time_;
  resetTimer();
  estimated_time_ = 0;
  return true;
}


void BSLMotionController::resetTimer() {
  if (task_timer_.isValid()) task_timer_.restart();
  else task_timer_.start();
}
void BSLMotionController::pauseTimer() {
  if (!task_timer_.isValid()) return;
  running_task_time_ -= task_timer_.elapsed();
  task_timer_.invalidate();
}
int BSLMotionController::getRemainingTime() {
  if (!task_timer_.isValid()) resetTimer();
  // Add addtional 3s for starting time and tolerance
  return running_task_time_ - task_timer_.elapsed() + 3000;
}