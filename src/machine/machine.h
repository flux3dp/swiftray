#ifndef MACHINE_H
#define MACHINE_H

#include <QObject>
#include <QSharedPointer>
#include <QPointer>
#include <QThread>
#include <QStringList>
#include <QProgressDialog>
#include <QPixmap>
#include <QSerialPort>
#include <settings/machine-settings.h>
#include <periph/motion_controller/motion_controller.h>
#include <executor/machine_job/machine_job.h>
#include <executor/machine_setup_executor.h>
#include <executor/job_executor.h>
#include <executor/rt_status_update_executor.h>
#include <executor/console_executor.h>

class Machine : public QObject
{
  Q_OBJECT
public:
  enum ConnectionState {
    kDisconnected, // Port closed
    kConnecting,   // Port open, but hasn't receive any response from machine
    kConnected     // Port open and has receive some response from machine
  };

  explicit Machine(MachineSettings::MachineParam mach, QObject *parent = nullptr);

  ConnectionState getConnectionState();
  bool applyMachineParam(MachineSettings::MachineParam mach);
  MachineSettings::MachineParam getMachineParam() const;
  bool createGCodeJob(const QStringList& gcode_list, const QList<Timestamp>& timestamp_list = QList<Timestamp>());
  bool createGCodeJob(const QStringList& gcode_list, const QList<Timestamp>& timestamp_list, QPixmap preview);
  bool createFramingJob(QStringList gcode_list);
  bool createRotaryTestJob(QRectF bbox, char rotary_axis, qreal feedrate, double framing_power);
  bool createJoggingRelativeJob(qreal x_dist, qreal y_dist, qreal z_dist, qreal feedrate);
  bool createJoggingAbsoluteJob(std::tuple<qreal, qreal, qreal> pos, qreal feedrate);
  bool createJoggingXAbsoluteJob(std::tuple<qreal, qreal, qreal> pos, qreal feedrate);
  bool createJoggingCornerJob(int corner_id, qreal feedrate);
  bool createJoggingEdgeJob(int edge_id, qreal feedrate);
  bool isConnected();
  void syncPosition();
  void setCustomOrigin(std::tuple<qreal, qreal, qreal> new_origin);
  void setSerialPort(QSerialPort &serial_port);
  bool connectSerial(QString portName, int baudrate);
  void disconnect();
  std::tuple<qreal, qreal, qreal> getCustomOrigin();
  std::tuple<qreal, qreal, qreal> getCurrentPosition();
  QPointer<MotionController> getMotionController() const { return motion_controller_; }
  QPointer<JobExecutor> getJobExecutor() const { return job_executor_; }
  QPointer<ConsoleExecutor> getConsoleExecutor() const { return console_executor_; }
  QPointer<RTStatusUpdateExecutor> getRTStatusUpdateExecutor() const { return rt_status_executor_; }

public Q_SLOTS:
  void motionPortActivated();  // Motion controller working
  void motionPortDisonnected();// Closed

  void handleNotif(QString title, QString msg);

  void startJob();
  void pauseJob();
  void resumeJob();
  void stopJob();

Q_SIGNALS:
  void connected();
  void activated();
  void disconnected();
  void positionCached(std::tuple<qreal, qreal, qreal>);

  void logSent(QString);
  void logRcvd(QString);

protected:
  MachineSettings::MachineParam machine_param_; // Settings for software, NOT the grbl settings
  ConnectionState connect_state_ = ConnectionState::kDisconnected;
  QSerialPort *serial_port_ = NULL;

  // Hardware equipment controllers
  MotionController *motion_controller_; // created when port connected
  //AutofocusController *af_controller_;
  //CameraController *camera_controller_;
  
  // Task Executors
  JobExecutor *job_executor_;
  ConsoleExecutor *console_executor_; // A general-purpose simple cmd executor
  MachineSetupExecutor *machine_setup_executor_;
  RTStatusUpdateExecutor *rt_status_executor_;
  //QThread *job_exec_thread_;
  //QThread *machine_setup_exec_thread_;
  //QThread *rt_status_exec_thread_;
  //UserCmdExecutor *user_cmd_executor_;
  //JoggingExecutor *jogging_executor_;

  qreal cached_x_pos_ = 0; // non-realtime, in canvas coord (independent of machine direction)
  qreal cached_y_pos_ = 0; // non-realtime, in canvas coord (independent of machine direction)
  qreal cached_z_pos_ = 0; // non-realtime, in canvas coord (independent of machine direction)

  bool bsl_connected_ = false;

  std::tuple<qreal, qreal, qreal> custom_origin_; // store the custom origin set by user

  std::tuple<qreal, qreal, qreal> canvasToMachineCoordConvert(std::tuple<qreal, qreal, qreal> pos, bool relative);
  std::tuple<qreal, qreal, qreal> machineToCanvasCoordConvert(std::tuple<qreal, qreal, qreal> pos, bool relative);

  void setupMotionController();  // Opened but not check
};

#endif // MACHINE_H
