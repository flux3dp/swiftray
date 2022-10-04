#include "job_executor.h"

#include <QDebug>

JobExecutor::JobExecutor(QObject *parent)
  : Executor{parent}
{
  qInfo() << "JobExecutor created";
  exec_timer_ = new QTimer(this); // trigger immediately when started
  connect(exec_timer_, &QTimer::timeout, this, &JobExecutor::exec);
  connect(this, &JobExecutor::trigger, this, &JobExecutor::wakeUp);
}

void JobExecutor::attachMotionController(
    QPointer<MotionController> motion_controller) {
  // In case running, stop first
  stop();
  motion_controller_ = motion_controller;
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
  std::scoped_lock<std::mutex> lk(exec_mutex_);

  // Reject if already runnign a job
  if (state_ != State::kIdle &&
      state_ != State::kCompleted &&
      state_ != State::kStopped) {
    qInfo() << "Unable to start executor, already running";
    return;
  }

  // Reject in case any operation is running
  if (motion_controller_->getState() != MotionControllerState::kIdle) {
    qInfo() << "Motion controller must be idle before job start";
    return;
  }

  // Next job selection rule:
  //   1. pendning_job_ = start new (next) job
  //   2. last_job_     = retry
  if (!pending_job_.isNull()) {
    active_job_ = pending_job_;
    pending_job_.reset();
    last_job_.reset();
  } else if (!last_job_.isNull()) {
    active_job_ = last_job_;
    last_job_.reset();
  } else {
    active_job_.reset();
    // Reject if no job available
    qInfo() << "JobExecutor no job available";
    return;
  }

  connect(motion_controller_, &MotionController::disconnected,
        this, &JobExecutor::stop);
  connect(motion_controller_, &MotionController::resetDetected,
        this, &JobExecutor::stop);
  // Register signal slot
  connect(motion_controller_, &MotionController::realTimeStatusUpdated, 
      this, [=](MotionControllerState last_state, MotionControllerState current_state, 
      qreal x_pos, qreal y_pos, qreal z_pos){
    // NOTE: Currently, state ofjob executor only depends on state of motion controller
    //       Thus, we update job executor state based on motion controller state
    //       allow the condition pause/resume/stop from other executor
    latest_mc_state_ = current_state;
    if (latest_mc_state_ == MotionControllerState::kAlarm ||
        latest_mc_state_ == MotionControllerState::kSleep) {
      qInfo() << "Stopped by alarm or sleep";
      stop();
    } else if (latest_mc_state_ == MotionControllerState::kIdle || 
                latest_mc_state_ == MotionControllerState::kRun) {
      resume();
    } else if (latest_mc_state_ == MotionControllerState::kPaused) {
      pause();
    } else { 
      // unhandled
    }
  });

  // Start running
  completed_cmd_cnt_ = 0;
  emit progressChanged(0);
  changeState(State::kRunning);
  wakeUp();
}

/**
 * @brief Pause the job executor
 *        NOTE: Do nothing to motion controller directly
 * 
 */
void JobExecutor::pause() {
  std::scoped_lock<std::mutex> lk(exec_mutex_);

  if (state_ == State::kRunning) {
    changeState(State::kPaused);
    exec_timer_->stop();
  }
}

/**
 * @brief Resume the job executor
 *        NOTE: Do nothing to motion controller directly
 * 
 */
void JobExecutor::resume() {
  std::scoped_lock<std::mutex> lk(exec_mutex_);

  if (state_ == State::kPaused) {
    changeState(State::kRunning);
  }
  wakeUp();
}

/**
 * @brief Main operation for job execution: send a single cmd
 *        only perform execution when in active state
 * 
 */
void JobExecutor::exec() {
  std::scoped_lock<std::mutex> lk(exec_mutex_);

  // 1. Check state first
  if (state_ != State::kRunning) {
    exec_timer_->stop();
    return;
  }

  // 2. Check end condition of job
  if (active_job_.isNull()) {
    exec_timer_->stop();
    return;
  }
  if (active_job_->end()) {
    if (!pending_cmd_ && cmd_in_progress_.isEmpty()) {
      if (latest_mc_state_ != MotionControllerState::kRun
          && latest_mc_state_ != MotionControllerState::kPaused) {
        // Complete
        qInfo() << "Job complete";
        complete();
        emit Executor::finished();
        return;
      }
    }
  }

  // 3. Prepare the next cmd
  if (!pending_cmd_) {
    if (active_job_->end()) {
      exec_timer_->stop();
      return;
    }
    pending_cmd_ = active_job_->getNextCmd();
  }

  // 4. Send (execute) cmd
  OperationCmd::ExecStatus exec_status = pending_cmd_->execute(this);
  if (exec_status == OperationCmd::ExecStatus::kIdle) {
    // Sleep (block) until next real-time status reported or cmd finished
    exec_timer_->stop();
    return;
  } else if (exec_status == OperationCmd::ExecStatus::kProcessing) {
    cmd_in_progress_.push_back(pending_cmd_);
    pending_cmd_.reset();
  } else { // Finish immediately: ok or error
    completed_cmd_cnt_ += 1;
    //if (completed_cmd_cnt_ % 25) {
      emit progressChanged(active_job_->getProgressPercent());
      emit elapsedTimeChanged(active_job_->getElapsedTime());
    //}
    pending_cmd_.reset();
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
  std::scoped_lock<std::mutex> lk(exec_mutex_);

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
  //if (completed_cmd_cnt_ % 25) {
    emit progressChanged(active_job_->getProgressPercent());
    emit elapsedTimeChanged(active_job_->getElapsedTime());
  //}
  emit trigger(); // ~ wakeUp() (might be triggered from different thread)
}

void JobExecutor::complete() {  
  if (!motion_controller_.isNull()) {
    disconnect(motion_controller_, nullptr, this, nullptr);
  }
  emit progressChanged(100);
  // Take out the active job to last job
  last_job_ = active_job_;
  if (!last_job_.isNull()) {
    last_job_->reload();
  }
  active_job_.reset();

  exec_timer_->stop();
  cmd_in_progress_.clear();
  pending_cmd_.reset();
  changeState(State::kCompleted);
}

/**
 * @brief Stop from class internal method
 * 
 */
void JobExecutor::stopImpl() {
  if (!motion_controller_.isNull()) {
    disconnect(motion_controller_, nullptr, this, nullptr);
  }
  // Take out the active job to last job
  emit progressChanged(0);
  last_job_ = active_job_;
  if (!last_job_.isNull()) {
    last_job_->reload();
  }
  active_job_.reset();

  exec_timer_->stop();
  cmd_in_progress_.clear();
  pending_cmd_.reset();
  changeState(State::kStopped);
}

/**
 * @brief Stop by other, protect with mutex
 * 
 */
void JobExecutor::stop() {
  std::scoped_lock<std::mutex> lk(exec_mutex_);
  stopImpl();
}

/**
 * @brief Wake up the work horse timer
 * 
 */
void JobExecutor::wakeUp() {
  if (!exec_timer_->isActive()) {
    exec_timer_->start();
  }
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
    return pending_job_->getTotalRequiredTime();
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
