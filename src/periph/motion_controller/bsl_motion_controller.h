#pragma once

#include "bsl_list_manager.h"
#include "motion_controller.h"

#include <QStringList>
#include <QRegularExpression>
#include <QElapsedTimer>
#include <mutex>
#include <thread>
#include <queue>
#include "liblcs/lcsApi.h"
#include "liblcs/lcsExpr.h"

struct TaskSettings {
  // Reset param before start list
  unsigned char current_s = 0;  // 0~100
  double current_f = 100.0;     // Default speed, mm/s
  double period = 100.0;        // us
  double q_pulse_width = 0.021; // us
  uint16_t pulse_width = 1;     // ns
  double wobble_diameter = -1;  // mm
  double wobble_step = 0;       // mm
  bool rotary_mode = false;
};

class BSLMotionController : public MotionController
{
  friend class BSLListManager;
public:
  BSLMotionController(QObject *parent = nullptr);
  ~BSLMotionController();
  void attachPortBSL();
  bool detachPort() override;
  bool resetState() override;
  QString type() override { return "BSL"; }
  CmdSendResult stop() override;
  CmdSendResult pause();
  CmdSendResult resume();
  CmdSendResult sendCmdPacket(QPointer<Executor> executor, QString cmd_packet) override;
  QString getCurrentError() { 
    if (!current_custom_error_.isNull()) return current_custom_error_;
    if (current_error_ != LCS_RES_NO_ERROR) return this->getErrorString(current_error_);
    return QString();
  }
  void setCorrection(double scaleX, double scaleY, double bucketX, double bucketY, double paralleX, double paralleY, double trapeX, double trapeY);
  void setScanaheadParams(double worksize, double angle, double xOffset, double yOffset);
  void setCheckDoor(bool check_door) { should_check_door_ = check_door; }
  void setTaskTime(double time) { total_task_time_ = time; }
  BoardRunStatus getBoardStatus();
  bool isConnected() override;
  bool isRunningLaser() { return is_running_laser_; }
  bool isFraming() { return is_framing_; }
  bool isHandlingReconnection() { return is_handling_reconnection_; }
  bool isPreparingFirstList() { return is_preparing_first_list_; }
  int getDisconnectCount() { return disconnect_count_; }
  double getProgressByTime();

public Q_SLOTS:
  void respReceived(QString resp) override;

private:
  void handleGcode(const QString &cmd_packet);
  void startCommandRunner();
  void commandRunnerThread();
  void dequeueCmd(int count);
  LCS2Error waitListAvailable(int list_no);
  QString getErrorString(int error_code);
  ListStatus getListStatus();
  void startList(int list_no, TaskSettings &settings, bool disable_laser);
  void setCo2Power(TaskSettings &settings);
  bool executeList(int list_no);
  void checkPauseResume();
  void resetTimer();
  void pauseTimer();
  int getRemainingTime();
  void jumpTo(double y, double x);
  void markTo(double y, double x);
  void setUpTaskCtrl();
  void setUpTaskList();

  std::queue<QString> pending_cmds_;
  std::mutex cmd_list_mutex_;
  bool is_running_laser_ = false;
  bool is_framing_ = false;
  bool is_handling_high_speed_ = false;
  bool is_threading = false;
  bool should_flush_ = false;
  bool lcs_paused_ = false;
  bool is_board_connected_ = false;
  bool is_handling_reconnection_ = false;
  bool is_preparing_first_list_ = false;
  bool before_first_laser_ = true;
  bool should_check_door_ = false;
  std::thread command_runner_thread_;
  int current_error_ = 0;
  QString current_custom_error_ = QString();
  int disconnect_count_ = -1;
  double high_speed_step_;
  int high_speed_data_count_ = 0;
  QString high_speed_data_;
  double total_task_time_ = 0; // Time for the whole job, ms
  double completed_task_time_ = 0; // Time for completed list + time before current list paused, ms
  double estimated_time_ = 0; // Time for current writing list, ms
  double running_task_time_ = 0; // Time for current executing list, ms
  QElapsedTimer task_timer_;
  BSLListManager list_manager_{this};
};