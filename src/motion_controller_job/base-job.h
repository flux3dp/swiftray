#pragma once

#include <QMutex>
#include <QThread>
#include <QVariant>
#include <QTime>

/**
    \class BaseJob
    \brief A class template for connection senders, we opensource this part to allow inserting GPL-licensed middleware
*/
class BaseJob : public QThread {
Q_OBJECT
public:
  enum class Status {
    READY,
    STARTING,
    RUNNING,
    PAUSED,
    ALARM,
    STOPPED,
    ALARM_STOPPED,
    FINISHED,
  };
  Q_ENUM(Status)

  BaseJob(QObject *parent, QString endpoint, QVariant data);

  ~BaseJob();

  virtual void start();

  virtual void stop();

  virtual void pause();

  virtual void resume();

  virtual int progress();

  Status status();

  static QString statusToString(Status status);

  void setTimestampList(QList<QTime> &&timestamp_list);
  void setTimestampList(const QList<QTime> &timestamp_list);
  QTime getTimestamp(int idx);
  QTime getTotalRequiredTime();
  QTime getElapsedTime();

  quint64 getFinishedCmdCnt() { return finished_cmd_cnt_; }
  inline quint64 getFinishedCmdIdx() { return finished_cmd_cnt_ == 0 ? 0 : (finished_cmd_cnt_ - 1); }

signals:

  void error(const QString &);
  void statusChanged(BaseJob::Status);
  void progressChanged(QVariant);
  void elapsedTimeChanged(QTime);

protected:

  void setStatus(Status status);
  inline void incFinishedCmdCnt() { finished_cmd_cnt_ += 1; }

  QMutex mutex_;

  Status status_ = Status::READY;

  QList<QTime> timestamp_list_;
  quint64 finished_cmd_cnt_ = 0;

};

//Q_DECLARE_METATYPE(BaseJob::Status);
