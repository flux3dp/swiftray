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

int lock_check = 0;
void BSLMotionController::handleGcode(QString gcode) {
    std::scoped_lock<std::mutex> lk(port_tx_mutex_);
    if (lock_check == 1) {
      qWarning() << "BSLMotionController::sendCmdPacket is locked";
      exit(-1);
    }
    lock_check = 1;
    static bool laserStarted = false;
    static bool laserEnabled = false;
    static double currentX = 0.0, currentY = 0.0;
    static double currentF = 6000.0; // Default speed
    static int currentS = 0; // Default power
    static bool absolutePositioning = true;
    static int listNo = 1;
    static int listBufferSize = 0;
    static int changeLimit = 0;
    static bool engravingBegun = false;
    //qInfo() << "Handle Gcode" << gcode;

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
              laserEnabled = true;
              lcs_set_laser_power(currentS / 10); // Assuming S1000 is 100% power
            } else {
              laserEnabled = false;
            }
        }
    }
    if (command == "G90") {
        absolutePositioning = true;
    } else if (command == "G91") {
        absolutePositioning = false;
    } else if (command == "M3" || command == "M4") {
        qInfo() << "Begin Laser";
        if (laserStarted) {
            qWarning() << "Laser already started";
            lock_check = 2;
            return;
        }
        laserStarted = true;
        laserEnabled = false;
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
        engravingBegun = false;
    } else if (command == "M2") {
        qInfo() << "Ending Laser";
        shouldSwapList = true;
        shouldEndLaser = true;
    } 

    if (listBufferSize > 600) {
        listBufferSize = 0;
        shouldSwapList = true;
    } else {
        listBufferSize++;
    }

    if (shouldSwapList) {
      qInfo("Setting end of list %d", listNo);
      lcs_set_end_of_list();
      // sleep 2ms to ensure the list is entirely loaded!
      QThread::msleep(2);
      qInfo("Executing list %d", listNo);
      lcs_execute_list(listNo);
      listNo = listNo == 1 ? 2 : 1;
      qInfo("Waiting for list to finish %d", listNo);
      LCS2Error ret = lcs_load_list(listNo, 0);
      while (ret != LCS_RES_NO_ERROR) {
        if (ret == LCS_GENERAL_CURRENTLY_BUSY) {
          qInfo("Currently busy", listNo);
        } else {
          qInfo("List Status %d", ret);
        }
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
      laserStarted = false;
      laserEnabled = false;
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

            if (laserEnabled && (command == "G1" || command.isEmpty())) {
                // qInfo() << "Mark X" << targetX << "Y" << targetY << "S" << currentS << "F" << currentF;
                lcs_mark_abs(-(targetY - 55), targetX - 55);
            } else {
                // qInfo() << "Jump X" << targetX << "Y" << targetY;
                lcs_jump_abs(-(targetY - 55), targetX - 55);
            }
            currentX = targetX;
            currentY = targetY;
        }
    }
    lock_check = 2;
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


QString BSLMotionController::getAlarmMsg(AlarmCode code) {
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
