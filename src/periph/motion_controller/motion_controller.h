#pragma once

#include <QObject>
#include <QSerialPort>
#include <QPointer>
#include <mutex>
#include <tuple>
#include <atomic>

class Executor;

enum class MotionControllerState {
  kUnknown, // default state
  kIdle,
  kRun,     // including RUN, JOG, HOMING
  kPaused,  // including HOLD, SAFETY_DOOR
  kAlarm,   
  kSleep,
  kCheck,
  kQuit
};

class MotionController : public QObject
{
  Q_OBJECT
public:
  enum class CmdSendResult {
    kOk,      // cmd sent (but hasn't been acked)
    kBusy,    // ask to try again later
    kInvalid, // rejected, invalid cmd format
    kFail     // Other error
  };

  explicit MotionController(QObject *parent = nullptr);

  void attachSerialPort(QSerialPort *port);
  virtual QString type() = 0;
  virtual bool detachPort() = 0;
  virtual bool resetState() = 0;
  virtual CmdSendResult sendCmdPacket(QPointer<Executor> executor, QString cmd_packet) = 0;
  virtual CmdSendResult stop() = 0;
  MotionControllerState getState() const;
  std::tuple<qreal, qreal, qreal> getPos() const;

Q_SIGNALS:
  void cmdSent(QString cmd);
  void respRcvd(QString resp);
  void resetDetected();
  void notif(QString title, QString msg);
  void statusUpdate(MotionControllerState state, qreal x, qreal y, qreal z);
  void disconnected();
  void stateChanged(MotionControllerState state);

public Q_SLOTS:
  virtual void respReceived(QString resp) = 0;

private Q_SLOTS:

protected:
  QSerialPort* port_ = nullptr;
  mutable std::mutex state_mutex_;
  QList<QPointer<Executor>> cmd_executor_queue_;
  void setState(MotionControllerState new_state);
  void enqueueCmdExecutor(QPointer<Executor>);
  void dequeueCmdExecutor();
  
  // Status info
  std::atomic<MotionControllerState> state_;  
  qreal x_pos_ = 0;
  qreal y_pos_ = 0;
  qreal z_pos_ = 0;
  //qreal a_pos_ = 0;

  QByteArray unprocssed_response_;
};