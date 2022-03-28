#pragma once

#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include <QSerialPort>
#include <QTimer>
#include <QByteArray>
#include <QVariant>

#include <connection/base-job.h>
#include <SerialPort/SerialPort.h>

//constexpr int kBlockBufferMax = 20; // Somehow pc don't need to manager block buffer?
constexpr int kGrblTimeout = 5000;  // timeout for normal GRBL gcode cmd
                                    // not suitable for $H (homing cmd)


enum class gcodeCmdCommState {
    kIdle = 0,
    kWaitingResp,
};

enum class ctrlCmd {
    kNull = 0,
    kReset,  // '\x18'
    kPause,  // '!'
    kResume, // '~'
    kRealTimeStatusReport // '?'
};
enum class ctrlCmdCommState {
    kIdle = 0,
    kWaitingResp // NOTE: only some of ctrl cmd have response
};
struct ctrlCmdState {
    ctrlCmd cmd;
    ctrlCmdCommState comm_state;
};

enum class systemCmd {
    kNull = 0,
    kUnlock, // $X
    kHoming, // $H
    kBuildInfo, // $I
    kHelpMsg, // $
    kGrblSettings, // $$
    //kGCodeParserState, // $G
    //kGCodeParam, // $#
};
enum class systemCmdCommState {
    kIdle = 0,
    kWaitingResp, // info or resp detail
    kWaitingOk,   // ok is an end condition of a cmd comm
};
struct systemCmdState {
    systemCmd cmd;
    systemCmdCommState comm_state;
};

class SerialJob : public BaseJob {
Q_OBJECT
public:
  SerialJob(QObject *parent, QString endpoint, const QVariant &gcode);
  SerialJob(QObject *parent, QString endpoint, QVariant &&gcode);

  ~SerialJob();

  void start() override;

  void stop() override;

  void pause() override;

  void resume() override;

  int progress() override;

  void parseResponse(QString line);

  bool isPaused();

signals:

  void startWaiting(int);
  void waitComplete();
#ifdef ENABLE_STATUS_REPORT
  void startStatusPolling();
  void endStatusPolling();
#endif

private slots:

  void timeout();

private:

  void run() override;

  //void receive();

  void startTimer(int ms);
  void stopTimer();

#ifdef ENABLE_STATUS_REPORT
  void onStartStatusPolling();
  void onStopStatusPolling();
  void onTimeToGetStatus();
  QTimer *realtime_status_report_timer_;
  bool is_starting_report_timer_; // use to handle the period between emit signal and start timer
#endif
  int serial_buffer_size_; // not necessary -> we only send the next gcode when ok is rcvd
  int planner_total_block_count_;
  int planner_block_unexecuted_count_;
  int plannerAvailableCnt() { return planner_total_block_count_ - planner_block_unexecuted_count_; }
  bool plannerBufferFull() { return planner_total_block_count_ == planner_block_unexecuted_count_; }


  //QByteArray unprocssed_response_;
  QString port_;
  QStringList gcode_;
  QTimer *timeout_timer_;

  int baudrate_;
  int current_line_;
  int progress_value_;
  unsigned long int wait_timeout_ = 1500;

  // action request flag
  bool pause_flag_;
  bool resume_flag_;
  bool stop_flag_;

  // cmd state
  gcodeCmdCommState gcode_cmd_comm_state_;
  ctrlCmdState ctrl_cmd_state_;
  systemCmdState system_cmd_state_;

  // meta status flag
  bool timeout_occurred_;
  bool waiting_first_ok_;
  bool grbl_reset_condition_detected_;
  int last_alarm_code_;

  void gcodeCmdNonblockingSend(std::string cmd);
  void ctrlCmdNonblockingSend(ctrlCmd cmd);
  void systemCmdBlockingSend(systemCmd cmd);
  void systemCmdNonblockingSend(systemCmd cmd);

};
