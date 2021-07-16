#pragma once

#include <QMutex>
#include <QThread>
#include <QVariant>

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
    PAUSING,
    RESUMING,
    FINISHED,
    ERROR_STOPPED,
    ERROR_PAUSED
  };

  BaseJob(QObject *parent, QString endpoint, QVariant data);

  ~BaseJob();

  virtual void start();

  virtual void pause();

  virtual void resume();

  virtual float progress();

  Status status();

signals:

  void error(const QString &error_message);

  void timeout(const QString &s);

protected:

  void setStatus(Status status);

  QMutex mutex_;

  Status status_;
};