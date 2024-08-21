#include "machine.h"
#include <QElapsedTimer>
#include <QtMath>
#include <QMessageBox>
#include <QSerialPort>
#include <tuple>

#include <settings/machine-settings.h>
#include <periph/motion_controller/motion_controller_factory.h>
#include <executor/machine_job/gcode_job.h>
#include <executor/machine_job/framing_job.h>
#include <executor/machine_job/jogging_job.h>
#include <executor/machine_job/rotary_test_job.h>
#include <executor/operation_cmd/grbl_cmd.h>
#include <executor/operation_cmd/bsl_cmd.h>
#include "liblcs/lcsExpr.h"

Machine::Machine(MachineSettings::MachineParam mach, QObject *parent)
  : QObject{parent}
{

  // Apply a default (placeholder) machine param
  applyMachineParam(mach);

  // Create executors belong to the machine
  job_executor_ = new JobExecutor{motion_controller_};
  machine_setup_executor_ = new MachineSetupExecutor{this};
  rt_status_executor_ = new RTStatusUpdateExecutor{this};
  console_executor_ = new ConsoleExecutor(motion_controller_);

  connect(rt_status_executor_, &RTStatusUpdateExecutor::hanging, [=]() {
    // TODO: Do something?
    qInfo() << "Realtime status updater hanging";
  });
  connect(machine_setup_executor_, &Executor::finished, this, &Machine::motionPortActivated);
  connect(job_executor_, &Executor::finished, this, &Machine::syncPosition); // Always sync the realtime position at the end of job
}

Machine::ConnectionState Machine::getConnectionState() {
  return connect_state_;
}

/**
 * @brief Apply (change) machine params to this machine controller
 * 
 * @param mach 
 */
bool Machine::applyMachineParam(MachineSettings::MachineParam mach) {
  machine_param_ = mach;
  qInfo() << "Apply Machine Param: " << mach.name << (int)mach.board_type;
  // NOTE: Currently, we only support Grbl machine, 
  //       thus, no need to re-create motion controller when MachineParam is changed
  //       However, when any other motion controller type is supported, we should handle it
  // TODO: 
  //  0. Determine whether accept new param when connected
  //  1. For single active machine design, 
  //         delete and create new motion controller, if already connected
  //  2. For multiple active machine design,
  //         ???
  return true;
}

MachineSettings::MachineParam Machine::getMachineParam() const { 
  return machine_param_; 
}

/**
 * @brief Create and set the gcode job as the next job
 * 
 * @param gcode_list 
 * @return true 
 * @return false: cancelled, no job executor exists or already running a job
 */
bool Machine::createGCodeJob(const QStringList& gcode_list, const QList<Timestamp>& timestamp_list) {
  return createGCodeJob(gcode_list, timestamp_list, QPixmap());
}

/**
 * @brief Create and set the gcode job as the next job
 * 
 * @param gcode_list 
 * @return true 
 * @return false: cancelled, no job executor exists or already running a job or machine not connected
 */
bool Machine::createGCodeJob(const QStringList& gcode_list, const QList<Timestamp>& timestamp_list, QPixmap preview) {
  // Check state
  if (connect_state_ != ConnectionState::kConnected) {
    return false;
  }

  auto job = preview.width() > 0 ? QSharedPointer<GCodeJob>::create(gcode_list, preview) : QSharedPointer<GCodeJob>::create(gcode_list);
  job->setMotionController(motion_controller_);
  job->setTimestampList(timestamp_list);
  if (!job_executor_) {
    return false;
  }
  if (!job_executor_->setNewJob(job)) {
    return false;
  }

  return true;
}

/**
 * @brief 
 * 
 * @param gcode_list 
 * @return true 
 * @return false: no job executor exists or already running a job
 */
bool Machine::createFramingJob(QStringList gcode_list) {
  // Check state
  if (connect_state_ != ConnectionState::kConnected) {
    return false;
  }

  auto job = QSharedPointer<FramingJob>::create(gcode_list);
  job->setMotionController(motion_controller_);
  if (!job_executor_) {
    return false;
  }
  if (!job_executor_->setNewJob(job)) {
    return false;
  }

  return true;
}

/**
 * @brief 
 * 
 * @param bbox 
 * @param rotary_axis 
 * @param feedrate 
 * @return true 
 * @return false 
 */
bool Machine::createRotaryTestJob(QRectF bbox, char rotary_axis, qreal feedrate, double framing_power) {
  // Check state
  if (connect_state_ != ConnectionState::kConnected) {
    return false;
  }

  auto job = QSharedPointer<RotaryTestJob>::create(bbox, rotary_axis, feedrate, framing_power);
  job->setMotionController(motion_controller_);
  if (!job_executor_) {
    return false;
  }
  if (!job_executor_->setNewJob(job)) {
    return false;
  }

  return true;
}

/**
 * @brief Create a Jogging Job for moving relatively
 * 
 * @param x_dist 
 * @param y_dist 
 * @param feedrate 
 * @return true 
 * @return false 
 */
bool Machine::createJoggingRelativeJob(qreal x_dist, qreal y_dist, qreal z_dist, qreal feedrate) {
  // Check state
  if (connect_state_ != ConnectionState::kConnected) {
    return false;
  }

  // Transform position according to origin position
  auto move = std::make_tuple(x_dist, y_dist, 0);
  move = canvasToMachineCoordConvert(move, true);
  auto job = QSharedPointer<JoggingRelativeJob>::create(
      std::get<0>(move), 
      std::get<1>(move), 
      feedrate
  );
  job->setMotionController(motion_controller_);
  if (!job_executor_) {
    return false;
  }
  if (!job_executor_->setNewJob(job)) {
    return false;
  }

  return true;
}

/**
 * @brief Create a Jogging Job for moving to absolute target point
 * 
 * @param target_pos 
 * @param feedrate 
 * @return true 
 * @return false 
 */
bool Machine::createJoggingAbsoluteJob(std::tuple<qreal, qreal, qreal> target_pos, 
                                        qreal feedrate) {
  // Check state
  if (connect_state_ != ConnectionState::kConnected) {
    return false;
  }
  // Transform position according to origin position
  target_pos = canvasToMachineCoordConvert(target_pos, false);
  auto job = QSharedPointer<JoggingAbsoluteJob>::create(true, std::get<0>(target_pos), 
                                                        true, std::get<1>(target_pos), 
                                                        feedrate);
  job->setMotionController(motion_controller_);
  if (!job_executor_) {
    return false;
  }
  if (!job_executor_->setNewJob(job)) {
    return false;
  }

  return true;
}

/**
 * @brief Create a Jogging Job for moving to absolute x position
 * 
 * @param x_pos 
 * @param feedrate 
 * @return true 
 * @return false 
 */
bool Machine::createJoggingXAbsoluteJob(std::tuple<qreal, qreal, qreal> target_pos, qreal feedrate) {
  // Check state
  if (connect_state_ != ConnectionState::kConnected) {
    return false;
  }
  // Transform position according to origin position
  target_pos = canvasToMachineCoordConvert(target_pos, false);
  auto job = QSharedPointer<JoggingAbsoluteJob>::create(true, std::get<0>(target_pos), 
                                                        false, 0, 
                                                        feedrate);
  job->setMotionController(motion_controller_);
  if (!job_executor_) {
    return false;
  }
  if (!job_executor_->setNewJob(job)) {
    return false;
  }

  return true;
}

/**
 * @brief Create a Jogging Job for moving to anchor point
 * 
 * @param corner_id 0: top left
 *                  1: top right
 *                  2: bottom left
 *                  3: bottom right
 * @param feedrate 
 * @return true 
 * @return false 
 */
bool Machine::createJoggingCornerJob(int corner_id, qreal feedrate) {
  // Check state
  if (connect_state_ != ConnectionState::kConnected) {
    return false;
  }

  auto target_pos = std::make_tuple<qreal, qreal, qreal>(0, 0, 0);
  if (corner_id == 0 || corner_id == 2) {
    std::get<0>(target_pos) = 0;
  } else {
    std::get<0>(target_pos) = machine_param_.width;
  }
  if (corner_id == 0 || corner_id == 1) {
    std::get<1>(target_pos) = 0;
  } else {
    std::get<1>(target_pos) = machine_param_.height;
  }
  
  // Transform position according to origin position
  target_pos = canvasToMachineCoordConvert(target_pos, false);
  qInfo() << "target pos: " << std::get<0>(target_pos) << ", " << std::get<1>(target_pos);
  auto job = QSharedPointer<JoggingAbsoluteJob>::create(true, std::get<0>(target_pos), true, std::get<1>(target_pos), feedrate);
  job->setMotionController(motion_controller_);
  if (!job_executor_) {
    return false;
  }
  if (!job_executor_->setNewJob(job)) {
    return false;
  }

  return true;
}

/**
 * @brief Create a Jogging Job for moving to anchor point
 * 
 * @param corner_id 0: right
 *                  1: top
 *                  2: left
 *                  3: bottom
 * @param feedrate 
 * @return true 
 * @return false 
 */
bool Machine::createJoggingEdgeJob(int edge_id, qreal feedrate) {
  // Check state
  if (connect_state_ != ConnectionState::kConnected) {
    return false;
  }

  std::tuple<qreal, qreal, qreal> target_pos = std::make_tuple(0, 0, 0);
  std::tuple<qreal, qreal, qreal> target_machine_pos = std::make_tuple(0, 0, 0);
  bool move_x = false;
  bool move_y = false;
  QSharedPointer<JoggingAbsoluteJob> job;
  switch (edge_id) {
    case 0: // to right
      std::get<0>(target_pos) = machine_param_.width;
      move_x = true;
      target_machine_pos = canvasToMachineCoordConvert(target_pos, false);
      break;
    case 1: // to top
      move_y = true;
      // Transform position according to origin position
      target_machine_pos = canvasToMachineCoordConvert(target_pos, false);
      break;
    case 2: // to left
      move_x = true;
      target_machine_pos = canvasToMachineCoordConvert(target_pos, false);
      break;
    case 3: // to bottom
    default:
      std::get<1>(target_pos) = machine_param_.height;
      move_y = true;
      target_machine_pos = canvasToMachineCoordConvert(target_pos, false);
      break;
  }
  job = QSharedPointer<JoggingAbsoluteJob>::create(move_x, std::get<0>(target_machine_pos), 
                                                  move_y, std::get<1>(target_machine_pos), 
                                                  feedrate);
  job->setMotionController(motion_controller_);
  if (!job_executor_) {
    return false;
  }
  if (!job_executor_->setNewJob(job)) {
    return false;
  }

  return true;
}

/**
 * @brief Sync the cached position with the realtime position from motion controller
 * 
 */
void Machine::syncPosition() {
  if (connect_state_ != ConnectionState::kConnected) {
    return;
  }

  auto cached_pos =
      machineToCanvasCoordConvert(motion_controller_->getPos(), false);
  cached_x_pos_ = std::get<0>(cached_pos);
  cached_y_pos_ = std::get<1>(cached_pos);
  cached_z_pos_ = std::get<2>(cached_pos);

  Q_EMIT positionCached(std::make_tuple(cached_x_pos_, cached_y_pos_, cached_z_pos_));
}

void Machine::setCustomOrigin(std::tuple<qreal, qreal, qreal> new_origin) {
  if (connect_state_ != ConnectionState::kConnected) {
    return;
  }
  custom_origin_ = new_origin;
}

std::tuple<qreal, qreal, qreal> Machine::getCustomOrigin() {
  return custom_origin_;
}

std::tuple<qreal, qreal, qreal> Machine::getCurrentPosition() {
  return std::make_tuple(cached_x_pos_, cached_y_pos_, cached_z_pos_);
}


void Machine::setupMotionController() {
  qInfo() << "Machine::setupMotionController()";

  if (motion_controller_) {
    motion_controller_->deleteLater();
    motion_controller_ = nullptr;
  }


  qInfo() << "Machine::setupMotionController() creating motion controller";
  motion_controller_ = MotionControllerFactory::createMotionController(machine_param_, this);
  connect(motion_controller_, &MotionController::disconnected, this, &Machine::motionPortDisonnected);
  connect(motion_controller_, &MotionController::notif, this, &Machine::handleNotif);
  connect(motion_controller_, &MotionController::cmdSent, this, &Machine::logSent);
  connect(motion_controller_, &MotionController::respRcvd, this, &Machine::logRcvd);


  qInfo() << "Machine::board_type: " << (int)machine_param_.board_type;
  if (machine_param_.board_type == MachineSettings::MachineParam::BoardType::GRBL_2020) {
    qInfo() << "Setup GRBL Motion Controller";
    motion_controller_->attachSerialPort(serial_port_);
  } else if (machine_param_.board_type == MachineSettings::MachineParam::BoardType::BSL_2024) {
    // TODO:BSL
    qInfo() << "Setup BSL Motion Controller";
    ((BSLMotionController*)motion_controller_)->attachPortBSL();
  }
  
  // Reset state variables
  connect_state_ = ConnectionState::kConnecting;
  cached_x_pos_ = 0;
  cached_y_pos_ = 0;
  cached_z_pos_ = 0;

  custom_origin_ = std::make_tuple<qreal, qreal, qreal>(0, 0, 0);

  // Attach motion_controller to executors
  rt_status_executor_->attachMotionController(motion_controller_);
  machine_setup_executor_->attachMotionController(motion_controller_);
  job_executor_->attachMotionController(motion_controller_);
  console_executor_->attachMotionController(motion_controller_);

  Q_EMIT connected();

  machine_setup_executor_->start();
}

void Machine::motionPortActivated() {
  qInfo() << "Machine::motionPortActivated()";
  connect_state_ = ConnectionState::kConnected;
  Q_EMIT activated();

  rt_status_executor_->start();
}

void Machine::motionPortDisonnected() {
  qInfo() << "Machine::motionPortDisonnected()";
  connect_state_ = ConnectionState::kDisconnected;

  // Reset state variables
  custom_origin_ = std::make_tuple<qreal, qreal, qreal>(0, 0, 0);
  
  // ...
  qInfo() << "Machine::motionPortDisonnected()";

  Q_EMIT disconnected();
}

void Machine::startJob() {
  if (connect_state_ != ConnectionState::kConnected) {
    qWarning() << "Machine::startJob() - machine is not connected";
    return;
  }
  qInfo() << "Machine::startJob()";
  job_executor_->start();
}

void Machine::pauseJob() {
  // Send pause cmd to motion controller
  if (console_executor_ && job_executor_) {
    if (job_executor_->getState() == Executor::State::kRunning) {
      // TODO: Add support other motion controller than grbl
      if (machine_param_.board_type == MachineSettings::MachineParam::BoardType::GRBL_2020) {
        console_executor_->appendCmd(
          GrblCmdFactory::createGrblCmd(GrblCmdFactory::CmdType::kCtrlPause)
        );
      } else if (machine_param_.board_type == MachineSettings::MachineParam::BoardType::BSL_2024) {
        console_executor_->appendCmd(
          BSLCmdFactory::createBSLCmd(BSLCmdFactory::CmdType::kCtrlPause)
        );
      }
    }
  }
}

void Machine::resumeJob() {
  // Send resume cmd to motion controller
  if (console_executor_ && job_executor_) {
    if (job_executor_->getState() == Executor::State::kPaused) {
      // TODO: Add support other motion controller than grbl
      if (machine_param_.board_type == MachineSettings::MachineParam::BoardType::GRBL_2020) {
        console_executor_->appendCmd(
          GrblCmdFactory::createGrblCmd(GrblCmdFactory::CmdType::kCtrlResume)
        );
      } else if (machine_param_.board_type == MachineSettings::MachineParam::BoardType::BSL_2024) {
        console_executor_->appendCmd(
          BSLCmdFactory::createBSLCmd(BSLCmdFactory::CmdType::kCtrlResume)
        );
      }
    }
  }
}

void Machine::stopJob() {
  // Send stop cmd to motion controller
  if (console_executor_ && job_executor_) {
    if (job_executor_->getState() == Executor::State::kRunning || 
        job_executor_->getState() == Executor::State::kPaused) {
      // TODO: Add support other motion controller than grbl
      if (machine_param_.board_type == MachineSettings::MachineParam::BoardType::GRBL_2020) {
        console_executor_->appendCmd(
          GrblCmdFactory::createGrblCmd(GrblCmdFactory::CmdType::kCtrlReset)
        );
      } else if (machine_param_.board_type == MachineSettings::MachineParam::BoardType::BSL_2024) {
        qInfo() << "Machine::stopJob";
        console_executor_->appendCmd(
          BSLCmdFactory::createBSLCmd(BSLCmdFactory::CmdType::kCtrlReset)
        );
      }
    }
  }
}

/**
 * @brief Apply the origin (direction) settings of machine
 * 
 * @param pos 
 * @param relative 
 * @return std::tuple<qreal, qreal, qreal> 
 */
std::tuple<qreal, qreal, qreal> Machine::canvasToMachineCoordConvert(
    std::tuple<qreal, qreal, qreal> pos, bool relative) 
{  
  switch (machine_param_.origin) {
    case MachineSettings::MachineParam::OriginType::FrontLeft: // invert Y-axis
      std::get<1>(pos) = relative ? -(std::get<1>(pos)) : (machine_param_.height - std::get<1>(pos));
      break;
    case MachineSettings::MachineParam::OriginType::FrontRight: // invert X-axis & Y-axis
      std::get<0>(pos) = relative ? -(std::get<0>(pos)) : (machine_param_.width - std::get<0>(pos));
      std::get<1>(pos) = relative ? -(std::get<1>(pos)) : (machine_param_.height - std::get<1>(pos));
      break;
    case MachineSettings::MachineParam::OriginType::RearRight: // invert X-axis
      std::get<0>(pos) = relative ? -(std::get<0>(pos)) : (machine_param_.width - std::get<0>(pos));
      break;
    case MachineSettings::MachineParam::OriginType::RearLeft: // Coordinate of canvas and machine matched
    default:
      break;
  }
  return pos;
}

/**
 * @brief Apply the origin (direction) settings of machine
 * 
 * @param pos 
 * @param relative 
 * @return std::tuple<qreal, qreal, qreal> 
 */
std::tuple<qreal, qreal, qreal> Machine::machineToCanvasCoordConvert(
    std::tuple<qreal, qreal, qreal> machine_pos, bool relative) 
{  
  // NOTE: The forward and backward convert are the same
  return canvasToMachineCoordConvert(machine_pos, relative);
}

/**
 * @brief Display notification from machine to user during job execution
 * 
 * @param title 
 * @param msg 
 */
void Machine::handleNotif(QString title, QString msg) {
  if (job_executor_->getState() == Executor::State::kRunning || 
        job_executor_->getState() == Executor::State::kPaused) {
    auto msgbox = new QMessageBox;
    //msgbox->setWindowTitle();
    msgbox->setText(title);
    msgbox->setInformativeText(msg);
    msgbox->show();
  }
}

bool Machine::connectSerial(QString port, int baudrate) {
  if (port == "BSL") {
    if (connect_state_ == ConnectionState::kConnected) {
      qWarning() << "BSL already connected";
      return false;
    } else if (lcs_connect()) {
      qInfo() << "LCS connected";
      this->setupMotionController();
      return true;
    } else {
      qWarning() << "LCS connect failed";
      return false;
    }
  } else {
    // Connects to regular serial port
    serial_port_->setPortName(port);
    serial_port_->setBaudRate(baudrate);
    serial_port_->open(QIODevice::ReadWrite);
    if (!serial_port_->isOpen()) {
      qWarning() << "Serial port open failed";
      return false;
    }
    qInfo() << "Serial port opened";
    serial_port_->clearError();
    this->setupMotionController();
  }
  return true;
}

void Machine::disconnect() {
  if (motion_controller_ != NULL) {
    qInfo() << "MotionController detaching port";
    motion_controller_->detachPort();
  }
  serial_port_->close();
}

bool Machine::isConnected() {
  return connect_state_ == ConnectionState::kConnected;
}

void Machine::setSerialPort(QSerialPort &serial_port) {
  serial_port_ = &serial_port;
}