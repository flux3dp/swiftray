#include "job_executor.h"

#include <QDebug>

JobExecutor::JobExecutor(QPointer<MotionController> motion_controller, QObject *parent)
  : Executor{parent}, motion_controller_{motion_controller}
{
  qInfo() << "JobExecutor created";
}

void JobExecutor::exec() {
  qInfo() << "JobExecutor exec(): start executing job";
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

  while (!error_occurred_) {
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
  while (!error_occurred_) {
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

bool JobExecutor::setNewJob(QSharedPointer<MachineJob> new_job) {
  pending_job_ = new_job;
  return true;
}

void JobExecutor::onCmdAcked() {
  std::lock_guard<std::mutex> lk(acked_cmd_cnt_mutex_);
  acked_cmd_cnt_ += 1;
}

bool JobExecutor::idle() {
  // All acked cmds have finished execution
  if (motion_controller_->getState() == MotionControllerState::kIdle) {
    return true;
  }
  // TODO: Check other controllers if exist

  return false;
}
