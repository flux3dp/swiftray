#include "job_executor.h"

#include <QThread>
#include <QDebug>
#include <QMessageBox>
#include <QCoreApplication>
#include <debug/debug-timer.h>
#include <atomic>

JobExecutor::JobExecutor(QObject *parent)
  : Executor{parent}
{
  qInfo() << "JobExecutor created";
}

JobExecutor::~JobExecutor() {
  this->running_ = false;
  if (this->exec_thread_.joinable()) {
    this->exec_thread_.join();
  }
}

void JobExecutor::threadFunction() {
  this->running_ = true;
  while (this->running_) {
      this->exec();
  }
}

void JobExecutor::attachMotionController(QPointer<MotionController> motion_controller) {
  handleStopped();
  this->motion_controller_ = motion_controller;
  this->changeState(State::kIdle);
}

MachineJob const *JobExecutor::getActiveJob() const {
  return active_job_.data();
}

/**
 * @brief Start or Replay job execution
 *        Setup executor and start workhorse timer
 * 
 */
void JobExecutor::start() {
  qInfo() << "JobExecutor::start() @" << getDebugTime();
  
  if (state_ != State::kIdle && state_ != State::kCompleted && state_ != State::kStopped) {
    qInfo() << "Unable to start executor, already running";
    return;
  }

  if (motion_controller_->getState() != MotionControllerState::kIdle) {
    qInfo() << "Motion controller must be idle before job start";
    return;
  }

  if (!pending_job_.isNull()) {
    active_job_ = pending_job_;
    pending_job_.reset();
    last_job_.reset();
  } else if (!last_job_.isNull()) {
    active_job_ = last_job_;
    last_job_.reset();
  } else {
    active_job_.reset();
    qInfo() << "JobExecutor has no job available";
    return;
  }

  // Connect all motion_controller_ signals and slots
  connect(motion_controller_, &MotionController::disconnected, this, &JobExecutor::handleDisconnect);
  connect(motion_controller_, &MotionController::cmdFinished,
        this, [=](QPointer<Executor> executor){
    if (executor.data() == this) {
      handleCmdFinish(0);
    }
  });
  connect(motion_controller_, &MotionController::resetDetected, this, &JobExecutor::handleStopped);
  connect(motion_controller_, &MotionController::realTimeStatusUpdated, this, &JobExecutor::handleMotionControllerStateChanged);

  
  completed_cmd_cnt_ = 0;
  Q_EMIT progressChanged(0);
  changeState(State::kRunning);

  if (!this->exec_thread_.joinable()) {
    this->exec_thread_ = std::thread(&JobExecutor::threadFunction, this);
  }
}

/**
 * @brief Pause the job executor
 *        NOTE: Do nothing to motion controller directly
 * 
 */
void JobExecutor::handlePaused() {
  std::lock_guard<std::mutex> lock(exec_mutex_);
  if (state_ == State::kRunning) {
    changeState(State::kPaused);
  }
}

void JobExecutor::handleResume() {
  std::lock_guard<std::mutex> lock(exec_mutex_);
  if (state_ == State::kPaused) {
    changeState(State::kRunning);
  }
}

/**
 * @brief Main operation for job execution: send a single cmd
 *        only perform execution when in active state
 * 
 */
int debug_count = 0;
void JobExecutor::exec() {
  std::lock_guard<std::mutex> lock(exec_mutex_);
  debug_count++;

  // Check if we need to ignore, or continue exec based on executor's state
  if (state_ != State::kRunning || 
      active_job_.isNull() ||
      motion_controller_->getState() == MotionControllerState::kSleep) {
    QThread::msleep(100);
    return;
  }

  // Check if the buffer is full
  if (cmd_in_progress_.length() > 9000) {
    if (debug_count % 40 == 0) {
      qInfo() << "JobExecutor::exec() - buffer full @" << getDebugTime();
    }
    QThread::msleep(10);
    return;
  }

  if (active_job_->end()) {
    if (!pending_cmd_ && cmd_in_progress_.isEmpty()) {
      if (latest_mc_state_ != MotionControllerState::kRun && latest_mc_state_ != MotionControllerState::kPaused) {
        qInfo() << "JobExecutor::exec() - completed @" << getDebugTime();
        if (!motion_controller_.isNull()) {
          disconnect(motion_controller_, nullptr, this, nullptr);
        }
        // Clear active job
        last_job_ = active_job_;
        if (!last_job_.isNull()) {
          last_job_->reload();
        }
        active_job_.reset();
        // Emit related completion signals
        changeState(State::kCompleted);
        Q_EMIT progressChanged(100);
        Q_EMIT Executor::finished();
        return;
      }
    }

    if (debug_count % 40 == 0) {
      qInfo() << "JobExecutor::exec() - Job has ended, yet waiting" << cmd_in_progress_.size()  << "commands @" << getDebugTime();
    }
  }
  if (!pending_cmd_) {
    pending_cmd_ = active_job_->getNextCmd();
  }

  OperationCmd::ExecStatus exec_status = pending_cmd_->execute(this, motion_controller_);
  switch(exec_status) {
    case OperationCmd::ExecStatus::kIdle:
      pending_cmd_.reset();
      return;
    case OperationCmd::ExecStatus::kProcessing:
      cmd_in_progress_.push_back(pending_cmd_);
      pending_cmd_.reset();
      break;
    default:
      completed_cmd_cnt_ += 1;
      Q_EMIT progressChanged(active_job_->getProgressPercent());
      Q_EMIT elapsedTimeChanged(active_job_->getElapsedTime());
      pending_cmd_.reset();
      break;
  }
}

/**
 * @brief Add a new job waiting for execute
 *        Reject if active job exists, 
 *        Override pending_job
 *        NOTE: Currently only allow one job in either active_job or pending_job
 * 
 * @param new_job 
 * @return true if added to pending list
 * @return false if unable to add new job to pending list
 */
bool JobExecutor::setNewJob(QSharedPointer<MachineJob> new_job) {
  if (!active_job_.isNull()) {
    return false;
  }
  pending_job_ = new_job;
  return true;
}

/**
 * @brief 
 * 
 * @param code 0 for success
 *             >0 for failure
 */
void JobExecutor::handleCmdFinish(int code) {
  std::lock_guard<std::mutex> lock(exec_mutex_);

  if (active_job_.isNull()) {
    return;
  }

  if (!cmd_in_progress_.isEmpty()) {
    if (code == 0) {
      cmd_in_progress_.first()->succeed();
    } else {
      cmd_in_progress_.first()->fail();
    }
    cmd_in_progress_.pop_front();
  }
  completed_cmd_cnt_ += 1;
  if (completed_cmd_cnt_ % 25 || cmd_in_progress_.length() < 100) {
    Q_EMIT progressChanged(active_job_->getProgressPercent());
    Q_EMIT elapsedTimeChanged(active_job_->getElapsedTime());
  }
}

/**
 * @brief Stop by other, protect with mutex
 * 
 */
void JobExecutor::handleStopped() {
  qInfo() << "JobExecutor::handleStopped()" << getDebugTime();
  std::lock_guard<std::mutex> lock(exec_mutex_);
  
  // Disconnect all motion_controller_ signals
  if (!motion_controller_.isNull()) {
    disconnect(motion_controller_, nullptr, this, nullptr);
  }

  // Clear active job
  last_job_ = active_job_;
  if (!last_job_.isNull()) {
    last_job_->reload();
  }
  active_job_.reset();

  // Clear pending commands
  cmd_in_progress_.clear();
  pending_cmd_.reset();

  Q_EMIT progressChanged(0);
  changeState(State::kStopped);

  // note: this should be not required anymore, since the control of MotionController state won't be changed other than JobExecutor::start()
  // if (!motion_controller_.isNull() && motion_controller_->getState() == MotionControllerState::kSleep) {
  //   motion_controller_->setState(MotionControllerState::kIdle);
  // }
}

/**
 * @brief Get the total required Time for active/pending/last job
 * 
 * @return Timestamp 
 */
Timestamp JobExecutor::getTotalRequiredTime() const {
  if (!active_job_.isNull()) {
    return active_job_->getTotalRequiredTime();
  } else if (!pending_job_.isNull()) {
    return pending_job_->getTotalRequiredTime();
  } else if (!last_job_.isNull()) {
    return last_job_->getTotalRequiredTime();
  }
  return Timestamp{};
}

/**
 * @brief Get progress of active job
 * 
 * @return float 
 */
float JobExecutor::getProgress() const {
  if (active_job_.isNull()) {
    return 0;
  }
  return active_job_->getProgressPercent();
}

/**
 * @brief Get elapsed time of active job
 * 
 * @return float 
 */
Timestamp JobExecutor::getElapsedTime() const {
  if (active_job_.isNull()) {
    return Timestamp{};
  }
  return active_job_->getElapsedTime();
}

void JobExecutor::reset() {
  if (state_ == State::kRunning || state_ == State::kPaused) {
    throw std::runtime_error("Unable to reset while running or paused");
  }
  changeState(State::kIdle);
}

void JobExecutor::handleDisconnect() {
  if (state_ == State::kRunning || state_ == State::kPaused) {
    auto msgbox = new QMessageBox;
    msgbox->setText(tr("Error"));
    msgbox->setInformativeText(tr("Serial port disconnected"));
    msgbox->show();
  }
  handleStopped();
}

void JobExecutor::handleMotionControllerStateChanged(MotionControllerState last_state, MotionControllerState current_state, 
                       qreal x_pos, qreal y_pos, qreal z_pos) {
  latest_mc_state_ = current_state;
  if (latest_mc_state_ == MotionControllerState::kAlarm || latest_mc_state_ == MotionControllerState::kSleep) {
    qInfo() << "Stopped by alarm or sleep";
    handleStopped();
  } else if (latest_mc_state_ == MotionControllerState::kIdle || latest_mc_state_ == MotionControllerState::kRun) {
    handleResume();
  } else if (latest_mc_state_ == MotionControllerState::kPaused) {
    handlePaused();
  }
}