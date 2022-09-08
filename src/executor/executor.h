#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <QObject>

class Executor : public QObject
{
  Q_OBJECT
public:
  explicit Executor(QObject *parent = nullptr);

public slots:
  virtual void start() = 0;
  virtual void exec() = 0;
  virtual void pause() = 0;
  virtual void resume() = 0; // resume from pause
  virtual void stop() = 0;

signals:
  void finished();
  //void error(QString err);

};

#endif // EXECUTOR_H
