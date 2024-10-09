#include "bsl_motion_controller.h"

#include <string>
#include <memory>
#include <thread>
#include <QDebug>
#include <QThread>
#include <executor/executor.h>
#include <debug/debug-timer.h>
#include <QApplication>
#include <QtCore/qcoreapplication.h>

#define MAX_BUFFER_LIST_SIZE 20000

int lcs_error_count = 0;

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
  qInfo() << "BSLM~::~BSLMotionController()";
  setState(MotionControllerState::kQuit);
  if (this->command_runner_thread_.joinable()) {
    this->command_runner_thread_.join();
  }
  qInfo() << "BSLM~::~BSLMotionController() - done";
}

void BSLMotionController::startCommandRunner() {
  qInfo() << "BSLM~::startCommandRunner()";
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
          lcs_pause_list();
          lcs_paused_ = true;
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
          lcs_restart_list();
          lcs_paused_ = false;
          QThread::msleep(25); 
          qInfo() << "BSLM~::thread() - resuming";
        }
        this->cmd_list_mutex_.lock();
        if (this->pending_cmds_.empty()) {
          if (debug_count_bsl % 40 == 1) qInfo() << "BSLM~::thread() - No pending commands, wait for a while";
          this->cmd_list_mutex_.unlock();
          this->should_flush_ = this->buffer_size_ > 0;
          setState(MotionControllerState::kIdle); // Set state to idle if there are no pending commands
          QThread::msleep(25);
        } else {
          if (debug_count_bsl % 1000 == 1) {
            qInfo() << "BSLM~::thread() - pending commands: " << this->pending_cmds_.size();
          }
          setState(MotionControllerState::kRun); // Set state to running if there are pending commands
          QString cmd = this->pending_cmds_.front();
          this->pending_cmds_.pop_front();
          this->cmd_list_mutex_.unlock();
          this->handleGcode(cmd);
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
  // qInfo() << "BSLM~::waitList(" << list_no << ")@" << getDebugTime();
  LCS2Error ret = lcs_load_list(list_no, 0);
  bool fixing_aready = false;
  while (ret != LCS_RES_NO_ERROR) {
    QThread::msleep(25);
    if (lcs_paused_) {
      continue;
    }
    qInfo() << getErrorString(ret);
    ret = lcs_load_list(list_no, 0);
      // If the list is already opened, close the list, execute it
    if (ret == LCS_GENERAL_AREADY_OPENED) {
      qInfo() << "BSLM~::waitListAvailable(" << list_no << ") - List already opened" << getDebugTime();
      if (!fixing_aready) {
        fixing_aready = true;
        lcs_set_end_of_list();
        lcs_execute_list(list_no);
      }
      ret = lcs_load_list(list_no, 0);
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

void BSLMotionController::handleGcode(const QString &gcode) {
    static bool rotary_mode = false;
    static bool laser_enabled = false;
    static int current_s = 0; // Default power
    static bool is_absolute_positioning = true;
    static int list_no = 1;
    static bool first_list = true;

    // Skip these GCode
    if (gcode == "\u0018" || gcode == "$I\n" || gcode == "$H\n") {
        dequeueCmd(1);
        return;
    }

    if (gcode == "?" || gcode == "?\n") {
      Q_EMIT MotionController::statusUpdate(state_, x_pos_, y_pos_, 0);
      // qInfo() << "BSLM~::handleGcode() - Realtime status updated" << getDebugTime();
      dequeueCmd(1);
      return;
    }

    QRegularExpression re("([GMXYFSZ])(-?\\d+\\.?\\d*)");
    QRegularExpressionMatchIterator i = re.globalMatch(gcode);

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
        } else if (type == "F") {
            current_f = value.toDouble();
            lcs_set_mark_speed_ctrl(current_f / 60.0); // Convert mm/min to mm/s
            lcs_set_jump_speed_ctrl(current_f / 60.0);
        } else if (type == "S") {
            current_s = value.toInt();
            if (current_s > 0) {
              laser_enabled = true;
              lcs_set_laser_power(current_s / 10); // Assuming S1000 is 100% power
            } else {
              laser_enabled = false;
            }
        }
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
          this->buffer_size_++;
          return;
      }
      is_running_laser_ = true;
      laser_enabled = false;
      list_no = 1;
      first_list = true;

      // Dump all lcs status
      ListStatus list_status;
      lcs_read_status((uint32_t *)&list_status);
      if (list_status.bMainOpen || list_status.bSubOepn || list_status.bCharOpen || list_status.bBusy1 || list_status.bBusy2 || list_status.bPaused || list_status.bLoop) {
        qInfo() << "BSLM~::handleGcode() - Irregular Status: " << list_status.bMainOpen << list_status.bSubOepn << list_status.bCharOpen << list_status.bLoop << list_status.bPaused << list_status.bBusy1 << list_status.bBusy2 << "@" << getDebugTime();
      }
      uint32_t running_pos;
      BoardRunStatus run_status;
      lcs_get_status((uint32_t *)&run_status, &running_pos);
      // qInfo() << "BSLM~::handleGcode() - Ready" << run_status.bCacheReady << "Running Pos: " << running_pos << "@" << getDebugTime();
      // Start new list
      lcs_set_jump_speed_ctrl(3000);
      lcs_set_mark_speed_ctrl(1000);
      lcs_set_laser_delays(-50, 50);
      lcs_set_start_list(1);
      lcs_set_laser_power(100);
      lcs_set_laser_mode(LCS_MOPA, false);
      lcs_enable_laser();
      lcs_error_count = 0;
      should_swap = false;
      should_end = false;
      should_flush_ = false;
    } else if (command == "M2") {
      // qInfo() << "BSLM~::handleGcode() - M2: Ending Laser Control" << getDebugTime();
      should_swap = true;
      should_end = true;
      rotary_mode = false;
    } else if (command == "M5") {
      qInfo() << "Turn Off Laser";
      dequeueCmd(1);
    } else if (command == "M100") { 
      rotary_mode = false;
      dequeueCmd(1);
    } else if (command == "M101") {
      rotary_mode = true;
      dequeueCmd(1);
    } else if (command == "M102") {
      qInfo() << "Enable OUT1/OUT2";
      lcs_write_io_port(0b1111);
      dequeueCmd(1);
    } else if (command == "M99" ) {
      char sn[50];
      lcs_get_serial_number(sn, 32);
      qInfo() << "BSLM~::handleGcode() - Serial Number: " << sn;
      Q_EMIT configUpdate("SN", sn);
      dequeueCmd(1);
    } else if (!is_move_command) {
      dequeueCmd(1);
      return;
    }

    this->buffer_size_++;
    
    if (this->buffer_size_ >= MAX_BUFFER_LIST_SIZE) {
        should_swap = true;
    }

    if (should_swap || is_running_laser_ && should_flush_) {
      should_flush_ = should_swap = false;
      // qInfo() << "BSLM~::handleGcode() - Flushing buffer with size" << this->buffer_size_ << "@" << getDebugTime();
      lcs_set_end_of_list();
      // qInfo() << "BSLM~::handleGcode() - Executing list" << list_no << "@" << getDebugTime();
      lcs_execute_list(list_no);
      first_list = false;
      list_no = list_no == 1 ? 2 : 1;
      waitListAvailable(list_no);
      lcs_set_start_list(list_no);
      // qInfo("BSLM~::handleGcode() - Swap new list %d", list_no);
      dequeueCmd(this->buffer_size_);
      this->buffer_size_ = 0;
      QThread::msleep(1);
    }

    if (should_end) {
      // qInfo() << "BSLM~::handleGcode() - Ending Laser Control"  << "@" << getDebugTime();
      lcs_set_end_of_list();
      // qInfo() << "BSLM~::handleGcode() - Executing list" << list_no << "@" << getDebugTime();
      lcs_execute_list(list_no);
      QThread::msleep(2);
      list_no = list_no == 1 ? 2 : 1;
      waitListAvailable(list_no); // Wait till the previous list is available.
      lcs_set_start_list(list_no);
      lcs_set_end_of_list();
      // qInfo() << "BSLM~::handleGcode() - Executing EMPTY list" << list_no << "@" << getDebugTime();
      lcs_execute_list(list_no);
      QThread::msleep(1);
      list_no = list_no == 1 ? 2 : 1;
      waitListAvailable(list_no); // Wait till the previous list is available.
      lcs_set_start_list(list_no);
      lcs_set_end_of_list();
      // qInfo() << "BSLM~::handleGcode() - Executing EMPTY list" << list_no << "@" << getDebugTime();
      lcs_execute_list(list_no);
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
      dequeueCmd(this->buffer_size_ + 1);
      is_running_laser_ = false;
      laser_enabled = false;
      should_end = false;
      lcs_disable_laser();
      lcs_set_laser_control(false);
    }

    // Process move command

    if (z != 0) {
      qInfo() << "BSLM~::handleGcode() - Z Axis" << z;
      //lcs_set_axis_move(1, z * 1600, false, 1600, 10, 255);
      should_swap = true;
      lcs_set_axis_move(0, z * 1600, false, 800, 10, 1000);
      // QThread::msleep(1000);
      dequeueCmd(1);
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
          lcs_set_axis_move(1, fabs(diff_y) * 100, diff_y < 0, 3200, 10, 255);
        }
        if (laser_enabled && (command == "G1" || command.isEmpty())) {
            // If target_x and target_y is near x_pos_ and y_pos_, jump and mark, if too far, engrave multiple points
            if ((pow(target_x - x_pos_, 2) + pow(target_y - y_pos_, 2)) > 0.1) {
                lcs_mark_abs(0, target_x - 55);
            } else {
                lcs_jump_abs(0, target_x - 55);
                lcs_laser_on_list(30);
            }
        } else {
            lcs_jump_abs(0, target_x - 55);
        }
      } else {
        if (laser_enabled && (command == "G1" || command.isEmpty())) {
            // If target_x and target_y is near x_pos_ and y_pos_, jump and mark, if too far, engrave multiple points
            if ((pow(target_x - x_pos_, 2) + pow(target_y - y_pos_, 2)) > 0.1) {
                lcs_mark_abs(-(target_y - 55), target_x - 55);
            } else {
                lcs_jump_abs(-(target_y - 55), target_x - 55);
                lcs_laser_on_list(100);
            }
        } else {
            lcs_jump_abs(-(target_y - 55), target_x - 55);
        }
      }
      x_pos_ = target_x;
      y_pos_ = target_y;
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
  this->pending_cmds_.push_back(cmd_packet);
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
  lcs_set_end_of_list();
  lcs_stop_execution();
  this->is_running_laser_ = false;
  this->pending_cmds_.clear();
  dequeueCmd(this->cmd_executor_queue_.size());
  Q_EMIT MotionController::resetDetected();
  QThread::msleep(2);
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