#pragma once

#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include <QSerialPort>
#include <QTimer>
#include <QByteArray>
#include <QVariant>
#include <list>

#include <motion_controller_job/base-job.h>
#include <connection/serial-port.h>

//constexpr int kBlockBufferMax = 20; // Somehow pc don't need to manager block buffer?
constexpr int kGrblTimeout = 5000;  // timeout for normal GRBL gcode cmd
                                    // not suitable for $H (homing cmd)


enum class GcodeCmdCommState {
    kIdle = 0,
    kWaitingResp,
};

enum class AlarmCode {
    kNone = 0,

    kMin = 1,

    kHardLimit = 1,
    kSoftLimit = 2,
    kAbortCycle = 3,
    kProbeFailInitial = 4,
    kProbeFailContact = 5,
    kHomingFailReset = 6,
    kHomingFailDoor = 7,
    kHomingFailPullOff = 8,
    kHomingFailApproach = 9,
    kHomingFailDualApproach = 10,

    kMax = 10,
};

enum class CtrlCmd {
    kNull = 0,
    kReset,  // '\x18'
    kPause,  // '!'
    kResume, // '~'
    kRealTimeStatusReport // '?'
};
enum class CtrlCmdCommState {
    kIdle = 0,
    kWaitingResp // NOTE: only some of ctrl cmd have response
};
struct CtrlCmdState {
    CtrlCmd cmd;
    CtrlCmdCommState comm_state;
};

enum class SystemCmd {
    kNull = 0,
    kUnlock, // $X
    kHoming, // $H
    kBuildInfo, // $I
    kHelpMsg, // $
    kGrblSettings, // $$
    //kGCodeParserState, // $G
    //kGCodeParam, // $#
};
enum class SystemCmdCommState {
    kIdle = 0,
    kWaitingResp, // info or resp detail
    kWaitingOk,   // ok is an end condition of a cmd comm
};
struct SystemCmdState {
    SystemCmd cmd;
    SystemCmdCommState comm_state;
};


class GrblJob : public BaseJob {
Q_OBJECT
public:
  GrblJob(QObject *parent, QString endpoint, const QVariant &gcode);
  GrblJob(QObject *parent, QString endpoint, QVariant &&gcode);

  ~GrblJob();

  void start() override;

  void stop() override;

  void pause() override;

  void resume() override;

  int progress() override;

  //void parseResponse(QString line);

  bool isPaused();

signals:
  void startTimeoutTimer(int);
  void stopTimeoutTimer();
  void startRealTimeStatusTimer(int);
  void stopRealTimeStatusTimer();

private slots:

  // NOTE: To modify the data member in QThread object, we should use a slot instead of changing it directly
  void timeout();
  void onPortDisconnected();
  void onResponseReceived(QString line);

  void onStartRealTimeStatusTimer(int ms);
  void onStopRealTimeStatusTimer();
  void onStartTimeoutTimer(int ms);
  void onStopTimeoutTimer();

  void onStatusReport();

private:

  void run() override;

  void handleRealtimeStatusReport(const QStringList& tokens);
  int serial_buffer_size_ = 80; // MUST be <= the actual Grbl buffer in machine
  int planner_total_block_count_ = 1;
  int planner_block_unexecuted_count_ = 0;
  int plannerAvailableCnt() { return planner_total_block_count_ - planner_block_unexecuted_count_; }
  bool plannerBufferFull() { return planner_total_block_count_ == planner_block_unexecuted_count_; }


  //QByteArray unprocssed_response_;
  QStringList gcode_;
  QTimer *real_time_status_timer_;
  QTimer *timeout_timer_;

  int current_line_ = 0;
  int progress_value_ = 0;
  unsigned long int wait_timeout_ = 1500;

  // cmd state
  GcodeCmdCommState gcode_cmd_comm_state_ = GcodeCmdCommState::kIdle;
  //ctrlCmdState ctrl_cmd_state_ = { .cmd = ctrlCmd::kNull, .comm_state = ctrlCmdCommState::kIdle};
  SystemCmdState system_cmd_state_ = { .cmd = SystemCmd::kNull, .comm_state = SystemCmdCommState::kIdle};

  // meta status flag
  bool timeout_occurred_ = false;
  bool waiting_first_ok_ = true;
  bool port_disconnected_ = false;
  bool status_report_blocked_ = false;
  AlarmCode last_alarm_code_ = AlarmCode::kNone;

  void gcodeCmdNonblockingSend(std::string cmd);
  void systemCmdNonblockingSend(SystemCmd cmd);

  std::list<QString> rcvd_lines_;
};
