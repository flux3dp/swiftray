#pragma once

#include "motion_controller.h"

#include <QStringList>
#include <QRegularExpression>
#include <mutex>
#include <thread>
#include "liblcs/lcsApi.h"
#include "liblcs/lcsExpr.h"

class BSLMotionController : public MotionController
{
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
  QString getCurrentError() { return this->getErrorString(current_error_); }
  void setCorrection(double scaleX, double scaleY, double bucketX, double bucketY, double paralleX, double paralleY, double trapeX, double trapeY);

public Q_SLOTS:
  void respReceived(QString resp) override;

private:
  void handleGcode(const QString &cmd_packet, bool force_pulse = false);
  void startCommandRunner();
  void commandRunnerThread();
  void dequeueCmd(int count);
  LCS2Error waitListAvailable(int list_no);
  QString getErrorString(int error_code);

  QStringList pending_cmds_;
  std::mutex cmd_list_mutex_;
  bool is_running_laser_ = false;
  bool is_handling_high_speed_ = false;
  bool is_threading = false;
  bool should_flush_ = false;
  bool lcs_paused_ = false;
  int buffer_size_ = 0;
  double current_x = 0.0;
  double current_y = 0.0;
  double current_f = 6000.0; // Default speed
  std::thread command_runner_thread_;
  int current_error_ = 0;
  double high_speed_step_;
  int high_speed_data_count_ = 0;
  QString high_speed_data_;
};