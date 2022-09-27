#ifndef MOTIONCONTROLLER_H
#define MOTIONCONTROLLER_H

#include <QObject>
#include <connection/serial-port.h>
#include <executor/executor.h>
#include <mutex>
#include <tuple>

enum class MotionControllerState {
  kUnknown, // default state
  kIdle,
  kRun,     // including RUN, JOG, HOMING
  kPaused,  // including HOLD, SAFETY_DOOR
  kAlarm,   
  kSleep,
  kCheck,
};


class MotionController : public QObject
{
  Q_OBJECT
public:
  enum class CmdSendResult {
    kOk,      // cmd sent
    kBusy,    // ask to try again later
    kInvalid, // rejected, invalid cmd format
    kFail     // Other error
  };

  explicit MotionController(QObject *parent = nullptr);

  void attachPort(SerialPort *port);
  virtual CmdSendResult sendCmdPacket(QPointer<Executor> executor, QString cmd_packet) = 0;
  MotionControllerState getState() const;
  void setState(MotionControllerState new_state);
  std::tuple<qreal, qreal, qreal> getPos() const;
  void enqueueCmdExecutor(QPointer<Executor>);
  void dequeueCmdExecutor();

signals:
  void cmdSent(QString cmd);
  void resetDetected();
  void realTimeStatusUpdated(MotionControllerState last_state, MotionControllerState new_state, 
      qreal x, qreal y, qreal z);
  void disconnected();

public slots:
  virtual void respReceived(QString resp) = 0;

private slots:

protected:

  SerialPort* port_;
  mutable std::mutex state_mutex_;
  QList<QPointer<Executor>> cmd_executor_queue_;
  
  // Status info
  MotionControllerState state_;  
  qreal x_pos_ = 0;
  qreal y_pos_ = 0;
  qreal z_pos_ = 0;
  //qreal a_pos_ = 0;
};

#endif // MOTIONCONTROLLER_H
