#ifndef MACHINECONTROLLER_H
#define MACHINECONTROLLER_H

#include <QObject>
#include <QSharedPointer>
#include <QWeakPointer>
#include <connection/serial-port.h>
#include <settings/machine-settings.h>
#include <motion_controller/motion-controller.h>

class MachineController : public QObject
{
  Q_OBJECT
public:
  explicit MachineController(const SerialPort *port, QSharedPointer<MotionController> motion_ctrler,
                             QObject *parent = nullptr);

  bool setNewJob(QSharedPointer<MachineJob> new_job);
  QWeakPointer<MotionController> getMotionController() { return motion_controller_.toWeakRef(); };


public slots:
  //void handleResponse();
  //void handleDisconnect();

signals:
  //void machineStateChanged(MachineState new_state);
  //void responseReceived(QString resp);

private slots:
  void send(QString raw_msg);

private:
  // ==== The life time of the followings >= MachineController ====
  const SerialPort *port_;
  QSharedPointer<MotionController> motion_controller_;
  // ===========================================================================
  QSharedPointer<MachineJob> current_job_;
  QMutex job_mutex_;
  MachineSettings::MachineSet machine_; // TODO: It should be a ref or pointer to the corresponding machine config
  QSharedPointer<OperationAgent> job_agent_;
  QSharedPointer<RealtimeStatusAgent> rt_status_agent_;
  //QSharedPointer<TerminalAgent> terminal_agent_;
  //QSharedPointer<JoggingAgent> jogging_agent_;
};

#endif // MACHINECONTROLLER_H
