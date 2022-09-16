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
  // In case already attached, detach first
  motion_controller_.clear();

  motion_controller_ = motion_controller;
  connect(motion_controller_, &MotionController::disconnected,
        this, &JobExecutor::stop);
}

/**
 * @brief Setup executor and start workhorse timer
 * 
 */
void JobExecutor::start() {
  qInfo() << "JobExecutor::start()";
  std::lock_guard<std::mutex> lk(exec_mutex_);

  if (state_ != State::kIdle &&
      state_ != State::kCompleted &&
      state_ != State::kStopped) {
    qInfo() << "Unable to start executor, already running";
    return;
  }

  // Clear the current job
  if (!active_job_.isNull()) {
    active_job_.reset();
  }
  // Next job selection order:
  //   1. pendning_job_ = start new job
  //   2. last_job_     = retry
  if (!pending_job_.isNull()) {
    active_job_ = pending_job_;
    pending_job_.reset();
    last_job_.reset();
  } else if (!last_job_.isNull()) {
    active_job_ = last_job_;
    last_job_.reset();
  } else {
    qInfo() << "JobExecutor active_job_ empty";
    return;
  }

  // Register signal slot
  connect(motion_controller_, &MotionController::realTimeStatusUpdated, 
      this, [=](MotionControllerState last_state, MotionControllerState current_state, 
      qreal x_pos, qreal y_pos, qreal z_pos, qreal a_pos){
    // NOTE: Currently, state ofjob executor only depends on state of motion controller
    //       Thus, we update job executor state based on motion controller state
    //       allow the condition pause/resume/stop from other executor
    latest_mc_state_ = current_state;
    if (latest_mc_state_ == MotionControllerState::kAlarm ||
        latest_mc_state_ == MotionControllerState::kSleep) {
      stop();
    } else if (latest_mc_state_ == MotionControllerState::kIdle || 
                latest_mc_state_ == MotionControllerState::kRun) {
      // try to resume if paused
      resume();
    } else if (latest_mc_state_ == MotionControllerState::kPaused) {
      pause();
    } else { 
      // unhandled
    }
  });

  // Start running
  repeat_ -= 1;
  changeState(State::kRunning);
  wakeUp();
}

/**
 * @brief Pause the job executor
 *        NOTE: Do nothing to motion controller directly
 * 
 */
void JobExecutor::pause() {
  std::lock_guard<std::mutex> lk(exec_mutex_);

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
  std::lock_guard<std::mutex> lk(exec_mutex_);

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
  std::lock_guard<std::mutex> lk(exec_mutex_);

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
        if (repeat_ <= 0) {
          // Complete
          qInfo() << "Job complete";
          complete();
          emit Executor::finished();
          return;
        } else {
          qInfo() << "Repeat again, " << repeat_ << " remaining";
          repeat_ -= 1;
          active_job_->reload();
        }
      }
    } else {
      qInfo() << "hasn't ended";
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
    pending_cmd_.reset();
  }

}

bool JobExecutor::setNewJob(QSharedPointer<MachineJob> new_job) {
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
  std::lock_guard<std::mutex> lk(exec_mutex_);
  
  // TODO: Seperate the cmds belong to different controllers
  if (!cmd_in_progress_.isEmpty()) {
    if (code == 0) {
      cmd_in_progress_.at(0)->succeed();
    } else {
      cmd_in_progress_.at(0)->fail();
    }
    cmd_in_progress_.pop_front();
  }
  emit trigger(); // ~ wakeUp() (might be triggered from different thread)
}

void JobExecutor::complete() {  
  if (!motion_controller_.isNull()) {
    disconnect(motion_controller_, nullptr, this, nullptr);
  }
  // Take out the active job to last job
  last_job_.reset();
  last_job_ = active_job_;
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
  last_job_.reset();
  last_job_ = active_job_;
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
  std::lock_guard<std::mutex> lk(exec_mutex_);
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

void JobExecutor::setRepeat(size_t repeat) {
  repeat_ = repeat;
}

size_t JobExecutor::getRepeat() {
  return repeat_;
}
