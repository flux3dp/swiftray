#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <QObject>

class Executor : public QObject
{
  Q_OBJECT
public:
  explicit Executor(QObject *parent = nullptr);

public slots:
  virtual void exec() = 0;

signals:
  void finished();
  //void error(QString err);

};

#endif // EXECUTOR_H
