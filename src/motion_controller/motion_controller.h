#ifndef MOTIONCONTROLLER_H
#define MOTIONCONTROLLER_H

#include <QObject>
#include <connection/serial-port.h>
#include <mutex>

enum class MotionControllerSystemCmd {
  // Grbl supported system cmd
  kViewParserState,   // $G
  kUnlock,            // $X
  kHome,              // $H
  kViewGrblSettings,  // $$
  kViewBuildInfo,     // $I
  kViewStartupBlocks, // $N
  kToggleCheckMode,   // $C
  // xxx supported system cmd
  // ...
};

enum class MotionControllerCtrlCmd {
  // Grbl supported ctrl cmd
  kStatusReport,      // ?
  kFeedHold,          // !
  kCycleStart,        // ~
  kSoftReset,         // 0x18
  // xxx supported ctrl cmd
  // ...
};

enum class MotionControllerState {
  kIdle,
  kRunning, // including CYCLE, JOG, HOMING
  kPaused,  // including HOLD, SAFETY_DOOR
  kAlarm,   
  kSleep,
  kCheck,
};


class MotionController : public QObject
{
  Q_OBJECT
public:
  explicit MotionController(QObject *parent = nullptr);

  void attachPort(SerialPort *port);
  virtual bool sendCmdPacket(QString cmd_packet) = 0;
  virtual bool sendSysCmd(MotionControllerSystemCmd sys_cmd) = 0;
  virtual bool sendCtrlCmd(MotionControllerCtrlCmd ctrl_cmd) = 0;
  MotionControllerState getState() const;

signals:
  void cmdSent(QString cmd);
  void ackRcvd();
  void realTimeStatusReceived();
  void stateChanged(MotionControllerState);
  void disconnected();

public slots:
  virtual void respReceived(QString resp) = 0;
  //virtual void handleRawCmd(QString raw_cmd);
  //virtual void handleCtrlCmd(MotionCtrlerCtrlCmd ctrl_cmd);
  //virtual void handleJoggingCmd(qreal x, qreal y, bool relative);

private slots:

protected:
  SerialPort* port_;
  mutable std::mutex state_mutex_;
  MotionControllerState state_;
};

#endif // MOTIONCONTROLLER_H
