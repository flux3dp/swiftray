#include "bsl_motion_controller.h"

#include <string>
#include <memory>
#include <thread>
#include <QDebug>
#include <QThread>
#include "liblcs/lcsApi.h"
#include "liblcs/lcsExpr.h"
#include <QtCore/qcoreapplication.h>

#define MAX_BUFFER_LIST_SIZE 3000

int lcs_error_count = 0;

void handleLCSError(LCS2Error error) {
  switch (error) {
    case LCS_RES_NO_ERROR:
      qInfo() << "LCS_RES_NO_ERROR";
      break;
    case LCS_GENERAL_ACTION_FAILED:
      qWarning() << "LCS_GENERAL_ACTION_FAILED";
      break;
    case LCS_GENERAL_ACTION_TIMEOUT:
      qWarning() << "LCS_GENERAL_ACTION_TIMEOUT";
      break;
    case LCS_GENERAL_INVALID_PARAM:
      qWarning() << "LCS_GENERAL_INVALID_PARAM";
      break;
    case LCS_GENERAL_OUT_OF_RANGE:
      qWarning() << "LCS_GENERAL_OUT_OF_RANGE";
      break;
    case LCS_GENERAL_MEMORY_NOT_ENOUGH:
      qWarning() << "LCS_GENERAL_MEMORY_NOT_ENOUGH";
      break;
    case LCS_GENERAL_BUFFER_SIZE_TOO_SMALL:
      qWarning() << "LCS_GENERAL_BUFFER_SIZE_TOO_SMALL";
      break;
    case LCS_GENERAL_WRITE_ERROR:
      qWarning() << "LCS_GENERAL_WRITE_ERROR";
      break;
    case LCS_GENERAL_READ_ERROR:
      qWarning() << "LCS_GENERAL_READ_ERROR";
      break;
    case LCS_GENERAL_UUID_EXIST:
      qWarning() << "LCS_GENERAL_UUID_EXIST";
      break;
    case LCS_GENERAL_UUID_NOT_EXIST:
      qWarning() << "LCS_GENERAL_UUID_NOT_EXIST";
      break;
    case LCS_GENERAL_CURRENTLY_BUSY:
      qWarning() << "LCS_GENERAL_CURRENTLY_BUSY";
      break;
    case LCS_GENERAL_PERMISSION_FAILED:
      qWarning() << "LCS_GENERAL_PERMISSION_FAILED";
      break;
    case LCS_GENERAL_NOT_INITIALIZED:
      qWarning() << "LCS_GENERAL_NOT_INITIALIZED";
      break;
    case LCS_GENERAL_NOT_OPENED:
      qWarning() << "LCS_GENERAL_NOT_OPENED";
      break;
    case LCS_GENERAL_AREADY_OPENED:
      qWarning() << "LCS_GENERAL_AREADY_OPENED";
      break;
    case LCS_BOARD_NOT_CONNECT:
      qWarning() << "LCS_BOARD_NOT_CONNECT";
      break;
    default:
      qWarning() << "Unknown LCS error";
      break;
  }
}

BSLMotionController::BSLMotionController(QObject *parent)
  : MotionController{parent}
{
  qInfo() << "BSLMotionController created";
  this->state_ = MotionControllerState::kIdle;
  this->should_flush_ = false;
}

void BSLMotionController::startCommandRunner() {
  this->command_runner_thread_ = std::thread(&BSLMotionController::commandRunnerThread, this);
}

void BSLMotionController::commandRunnerThread() {
  qInfo() << "BSLMotionController::commandRunnerThread() started";
  while (true) {
    if (this->state_ == MotionControllerState::kSleep) {
      qInfo() << "MotionController is in sleep state, ignore commandRunnerThread";
      QThread::msleep(100);
      continue;
    }
    this->cmd_list_mutex_.lock();
    if (this->pending_cmds_.empty()) {
      // qInfo() << "No pending commands, sleep for a while";
      this->cmd_list_mutex_.unlock();
      QThread::msleep(100);
      if (this->buffer_size_ > 0) {
        this->should_flush_ = true;
      }
      continue;
    }
    QString cmd = this->pending_cmds_.front();
    this->pending_cmds_.pop_front();
    this->cmd_list_mutex_.unlock();
    this->handleGcode(cmd);
  }
}

void BSLMotionController::dequeueCmd(int count) {
  this->cmd_list_mutex_.lock();
  // qInfo() << "Begin dequeueCmd with count" << count;
  for (int i = 0; i < count; i++) {
    if (!cmd_executor_queue_.isEmpty()) {
      auto cmd = cmd_executor_queue_.at(0);
      dequeueCmdExecutor();
      Q_EMIT cmdFinished(cmd);
    }
  }
  // qInfo() << "End dequeueCmd with count" << count;
  this->cmd_list_mutex_.unlock();
}

void BSLMotionController::handleGcode(const QString &gcode) {
    static bool laser_enabled = false;
    static double current_x = 0.0, current_y = 0.0, current_f = 6000.0; // Default speed
    static int current_s = 0; // Default power
    static bool is_absolute_positioning = true;
    static int list_no = 1;
    static bool first_list = true;

    if (this->state_ == MotionControllerState::kSleep) {
        qInfo() << "MotionController is in sleep state, ignore Gcode";
        return;
    }
    if (gcode == "\u0018" || gcode == "$I\n" || gcode == "?\n") {
        dequeueCmd(1);
        return;
    }

    QRegularExpression re("([GMXYFS])(-?\\d+\\.?\\d*)");
    QRegularExpressionMatchIterator i = re.globalMatch(gcode);

    bool is_move_command = false;
    bool should_swap = false;
    bool should_end = false;
    double x = is_absolute_positioning ? current_x : 0;
    double y = is_absolute_positioning ? current_y : 0;

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
        qInfo() << "[Laser Session Started]";
        if (is_running_laser_) {
            qWarning("Laser already started");
            this->buffer_size_++;
            return;
        }
        is_running_laser_ = true;
        laser_enabled = false;
        list_no = 1;
        first_list = true;
        lcs_set_jump_speed_ctrl(3000);
        lcs_set_mark_speed_ctrl(1000);
        lcs_set_laser_delays(-50, 50);
        lcs_set_laser_control(true);
        lcs_set_start_list(1);
        lcs_set_laser_power(100);
        lcs_set_laser_mode(LCS_FIBER, false);
        lcs_enable_laser();
        lcs_error_count = 0;
        setState(MotionControllerState::kRun);
    } else if (command == "M2") {
        qInfo("Ending Laser Control");
        should_swap = true;
        should_end = true;
    } 

    if (!is_running_laser_) {
      dequeueCmd(1);
      return;
    }

    this->buffer_size_++;
    
    if (this->buffer_size_ >= MAX_BUFFER_LIST_SIZE) {
        should_swap = true;
    }

    if (should_swap || is_running_laser_ && should_flush_) {
      should_flush_ = should_swap = false;
      qInfo("Closing list %d @ size %d", list_no, this->buffer_size_);
      if (first_list) {
        lcs_set_end_of_list();
        // lcs_auto_change();
        lcs_execute_list(list_no);
        first_list = false;
        list_no = list_no == 1 ? 2 : 1;
        qInfo("Waiting list #%d to be finished", list_no);
        LCS2Error ret = lcs_load_list(list_no, 0);
        while (ret != LCS_RES_NO_ERROR) {
          handleLCSError(ret);
          ret = lcs_load_list(list_no, 0);
          if (ret == LCS_GENERAL_AREADY_OPENED) {
            // If the list is already opened, close the list, execute it
            lcs_set_end_of_list();
            lcs_execute_list(list_no);
            ret = lcs_load_list(list_no, 0);
            break;
          }
          if (lcs_error_count ++ > 100) {
            qWarning("!!Too many errors, stop the laser");
            this->stop();
            break;
          }
          QThread::msleep(100);
        }
      } else {
        lcs_set_end_of_list();
        // lcs_auto_change();
        lcs_execute_list(list_no);
        list_no = list_no == 1 ? 2 : 1;
        qInfo("Waiting list #%d to be finished", list_no);
        LCS2Error ret = lcs_load_list(list_no, 0);
        while (ret != LCS_RES_NO_ERROR) {
          handleLCSError(ret);
          ret = lcs_load_list(list_no, 0);
          if (ret == LCS_GENERAL_AREADY_OPENED) {
            // If the list is already opened, close the list, execute it
            lcs_set_end_of_list();
            lcs_execute_list(list_no);
            ret = lcs_load_list(list_no, 0);
            break;
          }
          if (lcs_error_count ++ > 100) {
            qWarning("!!Too many errors, stop the laser");
            this->stop();
            break;
          }
          QThread::msleep(100);
        }
        if (ret != LCS_GENERAL_AREADY_OPENED) {
          lcs_set_start_list(list_no);
        }
        qInfo("Started new list %d", list_no);
      }
      dequeueCmd(this->buffer_size_);
      this->buffer_size_ = 0;
      QThread::msleep(2);
    }

    if (should_end) {
      qInfo("Finalizing Laser Session with remaining buffer %d", this->buffer_size_);
      lcs_set_end_of_list();
      // lcs_auto_change();
      list_no = list_no == 1 ? 2 : 1;
      lcs_execute_list(list_no);
      LCS2Error ret = lcs_load_list(list_no, 0);
      while (ret != LCS_RES_NO_ERROR) {
        handleLCSError(ret);
        ret = lcs_load_list(list_no, 0);
        if (ret == LCS_GENERAL_AREADY_OPENED) {
          qWarning("!!Final Aready opened");
          // If the list is already opened, just skip this error
          break;
        }
      }
      BoardRunStatus Status;
      do {
          uint32_t Pos;
          LCS2Error ret = lcs_get_status((uint32_t *)&Status, &Pos);

          if (ret != LCS_RES_NO_ERROR)
              break;
      } while (!Status.bCacheReady);
      qInfo("[Laser Session Closed]");
      qInfo() << "Pending commands: " << this->pending_cmds_.size();
      setState(MotionControllerState::kIdle);
      dequeueCmd(this->buffer_size_);
      is_running_laser_ = false;
      laser_enabled = false;
      should_end = false;
      lcs_disable_laser();
      lcs_set_laser_control(false);
    }

    // Process move command
    if (is_move_command) {
        double target_x, target_y;
        if (is_absolute_positioning) {
            target_x = x;
            target_y = y;
        } else {
            target_x = current_x + x;
            target_y = current_y + y;
        }

        if (laser_enabled && (command == "G1" || command.isEmpty())) {
            // If target_x and target_y is near current_x and current_y, jump and mark, if too far, engrave multiple points
            if ((pow(target_x - current_x, 2) + pow(target_y - current_y, 2)) > 0.1) {
                lcs_mark_abs(-(target_y - 55), target_x - 55);
            } else {
                lcs_jump_abs(-(target_y - 55), target_x - 55);
                lcs_laser_on_list(30);
            }
        } else {
            lcs_jump_abs(-(target_y - 55), target_x - 55);
        }
        current_x = target_x;
        current_y = target_y;
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
  if (!this->is_threading) {
    this->is_threading = true;
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
  qWarning() << "BSLMotionController::respReceived is not supported.";
  return;
}

// TODO:BSL
void BSLMotionController::attachPortBSL() {
  qInfo() << "MotionController::attachPortBSL() is not implemented well";
  setState(MotionControllerState::kIdle);
}

MotionController::CmdSendResult BSLMotionController::pause() {
  setState(MotionControllerState::kSleep);
  lcs_set_end_of_list();
  lcs_stop_execution();
  return CmdSendResult::kOk;
}

MotionController::CmdSendResult BSLMotionController::resume() {
  setState(MotionControllerState::kRun);
  return CmdSendResult::kOk;
}

MotionController::CmdSendResult BSLMotionController::stop() {
  qInfo() << "MotionController::stop()";
  setState(MotionControllerState::kSleep);
  lcs_set_end_of_list();
  lcs_stop_execution();
  QThread::sleep(2);
  is_running_laser_ = false;
  this->pending_cmds_.clear();
  Q_EMIT MotionController::resetDetected();
  return CmdSendResult::kOk;
}

QString BSLMotionController::getAlarmMsg(AlarmCode code) {
  switch (code) {
    case AlarmCode::kHardLimit:
      return tr("Hard limit");
    case AlarmCode::kSoftLimit:
      return tr("Soft limit");
    case AlarmCode::kAbortCycle:
      return tr("Abort during cycle");
    default:
      return "Unknown alarm";
  }
}
