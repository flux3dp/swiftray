#ifndef SERIALPORT_THREAD
#define SERIALPORT_THREAD

#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include <QSerialPort>

class SerialPortThread : public QThread {
Q_OBJECT
public:
  SerialPortThread(QObject *parent) : QThread(parent) {

  }

  ~SerialPortThread() {
    mutex_.lock();
    quit_ = true;
    cond_.wakeOne();
    mutex_.unlock();
    wait(wait_timeout_);
  }

  void playGcode(QString port, int baudrate, QStringList gcode);

signals:

  void response(const QString &s);

  void error(const QString &s);

  void timeout(const QString &s);

private:
  void run() override;

  QString port_;
  int baudrate_;

  unsigned long int wait_timeout_ = 1000;
  QMutex mutex_;
  QWaitCondition cond_;
  bool quit_ = false;

  QStringList gcode_;
  int current_line_ = 0;
};

#endif