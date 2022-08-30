#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <QObject>

class Executor : public QObject
{
  Q_OBJECT
public:
  explicit Executor(QObject *parent = nullptr);

  virtual void start() = 0;

signals:
  void finished();

};

#endif // EXECUTOR_H
