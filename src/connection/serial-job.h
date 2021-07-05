#ifndef SERIALPORT_THREAD
#define SERIALPORT_THREAD

#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include <QSerialPort>
#include <QTimer>
#include <QByteArray>
#include <QVariant>

#include <connection/base-job.h>

constexpr int kBlockBufferMax = 20; // Somehow pc don't need to manager block buffer?
constexpr int kGrblTimeout = 5000;

class SerialJob : public BaseJob {
Q_OBJECT
public:
  SerialJob(QObject *parent, QString endpoint, QVariant gcode);

  ~SerialJob();

  void start() override;

  void pause() override;

  void resume() override;

  float progress();

signals:

  void startConnection();

  void successConnection();

private slots:

  void timeout();

private:

  void run() override;

  void receive();

  void startTimer();

  void stopTimer();

  void parseResponse(QString line);


  QByteArray unprocssed_response_;
  QSerialPort *serial_;
  QString port_;
  QStringList gcode_;
  QTimer *timeout_timer_;
  bool grbl_ready_;
  int baudrate_;
  int block_buffer_;
  int current_line_;
  unsigned long int wait_timeout_ = 1000;

  bool pause_flag_;
  bool resume_flag_;
  bool on_hold_;
};

#endif