#include "job_executor.h"

#include <QDebug>

JobExecutor::JobExecutor(QObject *parent)
  : Executor{parent}
{
  qInfo() << "JobExecutor created";
  exec_timer_ = new QTimer(this); // trigger immediately when started
  connect(exec_timer_, &QTimer::timeout, this, &JobExecutor::exec);
}

void JobExecutor::attachMotionController(
    QPointer<MotionController> motion_controller) {
  if (!motion_controller_.isNull()) {
    // If already attached, detach first
    disconnect(motion_controller_, nullptr, this, nullptr);
    motion_controller_.clear();
    stop();
  }
  motion_controller_ = motion_controller;
}

/**
 * @brief Setup executor and start workhorse timer
 * 
 */
void JobExecutor::start() {
  qInfo() << "JobExecutor::start()";
  finishing_ = false;
  error_occurred_ = false;
  sent_cmd_cnt_ = 0;
  acked_cmd_cnt_ = 0;  // Updated in slot
  current_cmd_is_sent_ = true;
  block_until_all_acked_and_idle_ = false;
  current_cmd = std::make_tuple(Target::kNone, "");

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

  disconnect(motion_controller_, nullptr, this, nullptr);
  connect(motion_controller_, &MotionController::ackRcvd, this, &JobExecutor::onCmdAcked);
  connect(motion_controller_, &MotionController::stateUpdated, this, [=](MotionControllerState new_state){
    qInfo() << "stateUpdated slot";
    if (block_until_all_acked_and_idle_) {
      if (sent_cmd_cnt_ <= acked_cmd_cnt_ 
        && new_state != MotionControllerState::kRun
        && new_state != MotionControllerState::kPaused) {
        if (finishing_) {
          qInfo() << "Job finished: All cmd are acked and finished";
          disconnect(motion_controller_, nullptr, this, nullptr);
          emit Executor::finished();
          active_job_.reset();
        } else {
          // Unblock
          qInfo() << "Job unblocked";
          block_until_all_acked_and_idle_ = false;
          exec_timer_->start();
        }
      } else {
        // Remain in blocked state
        exec_timer_->stop();
        return;
      }
    }
  });

  exec_timer_->start();
}

/**
 * @brief Main operation for job execution: send a single cmd
 * 
 */
void JobExecutor::exec() {
  if (motion_controller_.isNull()) {
    stop();
    return;
  }

  if (active_job_.isNull()) {
    stop();
    return;
  }

  if (current_cmd_is_sent_ && active_job_->end()) {
    // End Condition: No cmd to be sent, wait for acked and finished
    qInfo() << "Job finishing: Cmd all are sent";
    // Force a wait for state update in case the last state is idle
    block_until_all_acked_and_idle_ = true;
    finishing_ = true;
    exec_timer_->stop();
    return;
  }

  if (current_cmd_is_sent_) {
    std::tuple<Target, QString> last_cmd = current_cmd;
    current_cmd = active_job_->getNextCmd();
    current_cmd_is_sent_ = false;
    if (std::get<0>(last_cmd) != std::get<0>(current_cmd)) { // Switching execution target
      block_until_all_acked_and_idle_ = true;
      exec_timer_->stop();
      return;
    }
    qInfo() << "Get new cmd:" << std::get<1>(current_cmd);
  }

  // 4. Send cmd
  bool send_success = true;
  switch (std::get<0>(current_cmd)) {
    case Target::kMotionControl:
      send_success = motion_controller_->sendCmdPacket(std::get<1>(current_cmd));
      break;
    default:
      break;
  }
  if (send_success) {
    sent_cmd_cnt_ += 1;
    qInfo() << "sent_cmd_cnt_: " << sent_cmd_cnt_;
    current_cmd_is_sent_ = true;
  } else {
    current_cmd_is_sent_ = false;
    // Stop exec until next resp (ack) or state change
    exec_timer_->stop();
    return;
  }

}

bool JobExecutor::setNewJob(QSharedPointer<MachineJob> new_job) {
  pending_job_ = new_job;
  return true;
}

void JobExecutor::onCmdAcked() {
  std::lock_guard<std::mutex> lk(acked_cmd_cnt_mutex_);
  acked_cmd_cnt_ += 1;
  qInfo() << "acked_cmd_cnt_: " << acked_cmd_cnt_;
  // Activate exec_timer_ again (if deactivated)
  exec_timer_->start();
}

bool JobExecutor::idle() {
  // All acked cmds have finished execution
  if (motion_controller_->getState() == MotionControllerState::kIdle) {
    return true;
  }
  // TODO: Check other controllers if exist

  return false;
}

void JobExecutor::stop() {
  stopped_ = true;
}
