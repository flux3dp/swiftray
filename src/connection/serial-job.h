#ifndef SERIALPORT_THREAD
#define SERIALPORT_THREAD

#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include <QSerialPort>
#include <QTimer>
#include <QByteArray>

#include <connection/base-job.h>

constexpr int kBlockBufferMax = 20; // Somehow pc don't need to manager block buffer?
constexpr int kGrblTimeout = 5000;

class SerialJob : public BaseJob {
Q_OBJECT
public:
  SerialJob(QObject *parent, QString port, int baudrate, QStringList gcode) : BaseJob(parent) {
    port_ = port;
    baudrate_ = baudrate;
    gcode_ = gcode;
    grbl_ready_ = false;
    timeout_timer_ = new QTimer(this);
    connect(this, &SerialJob::startConnection, this, &SerialJob::startTimer);
    connect(this, &SerialJob::successConnection, this, &SerialJob::stopTimer);
    connect(timeout_timer_, &QTimer::timeout, this, &SerialJob::timeout);
  }

  ~SerialJob() {
    mutex_.lock();
    mutex_.unlock();
    wait(wait_timeout_);
  }

  void start() {
    const QMutexLocker locker(&mutex_);
    if (!isRunning()) {
      QThread::start();
    } else {
      Q_ASSERT_X(false, "SerialJob", "Already running");
    }
  }

  float progress() override {
    return current_line_ / gcode_.size();
  }

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

  QSerialPort *serial_;
  QString port_;
  int baudrate_;

  unsigned long int wait_timeout_ = 1000;
  QStringList gcode_;
  int current_line_;

  QByteArray unprocssed_response_;

  bool grbl_ready_;

  int block_buffer_;

  QTimer *timeout_timer_;
};

#endif