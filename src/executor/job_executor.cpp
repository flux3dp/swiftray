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
  connect(motion_controller_, &MotionController::ackRcvd, this, &JobExecutor::onCmdAcked);
  
  // TODO: Change the following
  connect(motion_controller_, &MotionController::stateChanged, this, [=](){
      exec_timer_->start();
  });
}

void JobExecutor::start() {
  qInfo() << "JobExecutor::start()";
  error_occurred_ = false;
  sent_cmd_cnt_ = 0;
  acked_cmd_cnt_ = 0;  // Updated in slot
  current_cmd_is_sent_ = true;
  block_until_idle_ = false;
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
  exec_timer_->start();
}

void JobExecutor::exec() {
  if (motion_controller_.isNull()) {
    stop();
    return;
  }

  if (active_job_.isNull()) {
    stop();
    return;
  }

  if (current_cmd_is_sent_) {
    if (active_job_->end()) { // Job cmd all sent, wait for end condition to be met
      if (sent_cmd_cnt_ > acked_cmd_cnt_) {
        // All cmd have been sent, wait for ack
        exec_timer_->stop();
        return;
      } else {
        // All cmd have been sent and acked, wait for motion to finish
        block_until_idle_ = true;
      }
    } else { // Job hasn't ended, get the next cmd
      std::tuple<Target, QString> last_cmd = current_cmd;
      current_cmd = active_job_->getNextCmd();
      current_cmd_is_sent_ = false;
      if (std::get<0>(last_cmd) != std::get<0>(current_cmd)) { // Switching execution target
        block_until_idle_ = true;
      }
      qInfo() << "Get new cmd:" << std::get<1>(current_cmd);
    }
  }

  // Handle blocked condition
  if (block_until_idle_) {
    if (motion_controller_->getState() != MotionControllerState::kRunning
      && motion_controller_->getState() != MotionControllerState::kPaused) {
      if (active_job_->end() && sent_cmd_cnt_ <= acked_cmd_cnt_) {
        emit Executor::finished();
        active_job_.reset();
        return;
      }
    } else { // Stop exec until next resp (ack) or state change
      exec_timer_->stop();
      return;
    }
  }

  // Check if current cmd needs to be sent
  if (current_cmd_is_sent_) {
    // Stop exec until next resp (ack) or state change
    exec_timer_->stop();
    return;
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
    current_cmd_is_sent_ = true;
  } else {
    current_cmd_is_sent_ = false;
    // Stop exec until next resp (ack) or state change
    exec_timer_->stop();
    return;
  }

}

/*
void JobExecutor::exec() {
  stopped_ = false;
  qInfo() << "JobExecutor exec(): start executing job";

  if (motion_controller_.isNull()) {
    stop();
    return;
  }
  
  if (!active_job_.isNull()) {
    emit Executor::finished();
    return;
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
  }
  if (active_job_.isNull()) {
    qInfo() << "JobExecutor active_job_ empty";
    emit Executor::finished();
    return;
  }

  enum class BlockCondition {
    kNone,
    kForNextAck,
    kForAllAckedAndIdle
  };

  // TODO: Apply job repeat

  std::tuple<Target, QString> last_cmd = std::make_tuple(Target::kNone, ""); // last cmd sent
  std::tuple<Target, QString> next_cmd = std::make_tuple(Target::kNone, ""); // next cmd to be sent
  //bool exec_blocked = false; // whether cmd sending is blocked (paused)
  BlockCondition block_cond = BlockCondition::kNone;
  //bool resend_required = false;

  while (!error_occurred_ && !stopped_) {
    // 0. Check end of job
    if (active_job_->end() && acquired_cmd_cnt_ == sent_cmd_cnt_) {
      // No cmd need to be sent -> proceed to the next stage
      block_cond = BlockCondition::kForAllAckedAndIdle;
      break;
    }
    // 1. Wait until block condition is cleared
    if (block_cond == BlockCondition::kForAllAckedAndIdle) {
      if (acquired_cmd_cnt_ == acked_cmd_cnt_ && idle()) {
        block_cond = BlockCondition::kNone;
      }
    }
    if (block_cond != BlockCondition::kNone) {
      continue;
    }
    // 2. Acquire the next cmd if needed
    if (acquired_cmd_cnt_ == sent_cmd_cnt_) { // All acquired cmd have been sent, get next
      last_cmd = next_cmd;
      next_cmd = active_job_->getNextCmd();
      acquired_cmd_cnt_ += 1;
      if (std::get<0>(last_cmd) != std::get<0>(next_cmd)) { // Switching execution target
        block_cond = BlockCondition::kForAllAckedAndIdle; // Wait until all the in-progress cmd to finish
        continue;
      }
    }
    // 3. Determine action based on next_cmd and then execute
    bool send_success = true;
    switch (std::get<0>(next_cmd)) {
      case Target::kMotionControl:
        send_success = motion_controller_->sendCmdPacket(std::get<1>(next_cmd));
        break;
      default:
        break;
    }
    if (send_success) {
      sent_cmd_cnt_ += 1;
    }

  }

  // All cmd have been sent -> wait for ack and finish
  while (!error_occurred_ && !stopped_) {
    if (block_cond == BlockCondition::kForAllAckedAndIdle) {
      if (acquired_cmd_cnt_ == acked_cmd_cnt_ && idle()) {
        break;
      }
    }
  }

  qInfo() << "JobExecutor finish execute job";
  last_job_ = active_job_;
  active_job_.reset();
  last_job_->reload();
  emit Executor::finished();
  return;
}
*/

bool JobExecutor::setNewJob(QSharedPointer<MachineJob> new_job) {
  pending_job_ = new_job;
  return true;
}

void JobExecutor::onCmdAcked() {
  std::lock_guard<std::mutex> lk(acked_cmd_cnt_mutex_);
  acked_cmd_cnt_ += 1;
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
