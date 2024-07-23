#include "bsl_motion_controller.h"

#include <string>
#include <memory>
#include <QDebug>
#include <QThread>
#include "liblcs/lcsApi.h"
#include "liblcs/lcsExpr.h"
#include <QtCore/qcoreapplication.h>

BSLMotionController::BSLMotionController(QObject *parent)
  : MotionController{parent}
{
  qInfo() << "BSLMotionController created";
}

void BSLMotionController::handleGcode(QString gcode) {
    std::scoped_lock<std::mutex> lk(port_tx_mutex_);
    if (this->state_ == MotionControllerState::kSleep) {
        qInfo() << "MotionController is in sleep state, ignore Gcode";
        return;
    }
    static bool laser_enabled = false;
    static double currentX = 0.0, currentY = 0.0;
    static double currentF = 6000.0; // Default speed
    static int currentS = 0; // Default power
    static bool absolutePositioning = true;
    static int listNo = 1;
    static int listBufferSize = 0;
    // qInfo() << "Handle Gcode" << gcode;

    // Improved regex for G-code parsing
    QRegularExpression re("([GMXYFS])(-?\\d+\\.?\\d*)");
    QRegularExpressionMatchIterator i = re.globalMatch(gcode);

    bool hasMove = false;
    bool shouldSwapList = false;
    bool shouldEndLaser = false;
    double x = currentX, y = currentY;
    if (!absolutePositioning) {
        x = 0;
        y = 0;
    }
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
            hasMove = true;
        } else if (type == "Y") {
            y = value.toDouble();
            hasMove = true;
        } else if (type == "F") {
            currentF = value.toDouble();
            lcs_set_mark_speed_ctrl(currentF / 60.0); // Convert mm/min to mm/s
            lcs_set_jump_speed_ctrl(currentF / 60.0);
        } else if (type == "S") {
            currentS = value.toInt();
            if (currentS > 0) {
              laser_enabled = true;
              lcs_set_laser_power(currentS / 10); // Assuming S1000 is 100% power
            } else {
              laser_enabled = false;
            }
        }
    }
    if (command == "G90") {
        absolutePositioning = true;
    } else if (command == "G91") {
        absolutePositioning = false;
    } else if (command == "M3" || command == "M4") {
        qInfo() << "Begin Laser Control";
        if (is_running_laser_) {
            qWarning() << "Laser already started";
            return;
        }
        is_running_laser_ = true;
        laser_enabled = false;
        // Begin laser control
        // Set Laser mode
        // Disable laser delays
        // lcs_set_delay_mode(true, 0, 0, 50);
        // Enable Laser
        // Set power
        listNo = 1;
        lcs_set_jump_speed_ctrl(3000);
        lcs_set_mark_speed_ctrl(1000);
        lcs_set_laser_delays(-100, 100);
        lcs_set_laser_control(true);
        lcs_set_start_list(1);
        lcs_set_laser_power(100);
        lcs_set_laser_mode(LCS_FIBER, false);
        lcs_enable_laser();
    } else if (command == "M2") {
        qInfo() << "End Laser Control";
        shouldSwapList = true;
        shouldEndLaser = true;
    } 

    if (listBufferSize > 500) {
        listBufferSize = 0;
        shouldSwapList = true;
    } else {
        listBufferSize++;
    }

    if (shouldSwapList) {
      qInfo("Setting end of list %d", listNo);
      lcs_set_end_of_list();
      // sleep 2ms to ensure the list is entirely loaded!
      QThread::msleep(4);
      qInfo("Executing list %d", listNo);
      lcs_execute_list(listNo);
      listNo = listNo == 1 ? 2 : 1;
      qInfo("Waiting for list to finish %d", listNo);
      LCS2Error ret = lcs_load_list(listNo, 0);
      while (ret != LCS_RES_NO_ERROR) {
        ret == LCS_GENERAL_CURRENTLY_BUSY ? qInfo("*BUSY %d", listNo) : qInfo("List Status %d", ret);
        ret = lcs_load_list(listNo, 0);
        if (ret == LCS_GENERAL_AREADY_OPENED) {
          qInfo("Break because already opened");
          break;
        }
        QCoreApplication::processEvents();
      }
      qInfo("Waiting done");
      lcs_set_start_list(listNo);
      qInfo("Started new list %d", listNo);
    }

    if (shouldEndLaser) {
      lcs_set_end_of_list();
      lcs_execute_list(listNo);
      listNo = listNo == 1 ? 2 : 1;
      while (lcs_load_list(listNo, 0) != LCS_RES_NO_ERROR);
      BoardRunStatus Status;

      do {
          uint32_t Pos;
          LCS2Error ret = lcs_get_status((uint32_t *)&Status, &Pos);

          if (ret != LCS_RES_NO_ERROR)
              break;
      } while (!Status.bCacheReady);
      qInfo() << "End Laser";
      is_running_laser_ = false;
      laser_enabled = false;
      shouldEndLaser = false;
      lcs_disable_laser();
      lcs_set_laser_control(false);
    }
    if (command.startsWith("G") || hasMove) {
        if (hasMove) {
            double targetX, targetY;
            if (absolutePositioning) {
                targetX = x;
                targetY = y;
            } else {
                targetX = currentX + x;
                targetY = currentY + y;
            }

            if (laser_enabled && (command == "G1" || command.isEmpty())) {
                // qInfo() << "Mark X" << targetX << "Y" << targetY << "S" << currentS << "F" << currentF;
                // If targetX and targetY is near currentX and currentY, jump and mark, if too far, engrave multiple points
                if ((pow(targetX-currentX, 2) + pow(targetY-currentY, 2)) > 0.1) {
                    // qInfo() << "Engrave X" << targetX << "Y" << targetY;
                    lcs_mark_abs(-(targetY - 55), targetX - 55);
                } else {
                    // qInfo() << "Jump X" << targetX << "Y" << targetY;
                    lcs_jump_abs(-(targetY - 55), targetX - 55);
                    lcs_laser_on_list(100);
                }
            } else {
                // qInfo() << "Jump X" << targetX << "Y" << targetY;
                lcs_jump_abs(-(targetY - 55), targetX - 55);
            }
            currentX = targetX;
            currentY = targetY;
        }
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
  this->handleGcode(cmd_packet);
  return CmdSendResult::kFail;
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
}

MotionController::CmdSendResult BSLMotionController::resume() {
}

MotionController::CmdSendResult BSLMotionController::stop() {
  setState(MotionControllerState::kSleep);
  lcs_set_end_of_list();
  lcs_stop_execution();
  QThread::sleep(2);
  Q_EMIT MotionController::resetDetected();
  is_running_laser_ = false;
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
