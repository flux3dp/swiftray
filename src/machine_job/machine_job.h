#ifndef MACHINEJOB_H
#define MACHINEJOB_H

#include <QObject>
#include <QString>
#include <mutex>
#include <tuple>

enum class Target {
  kMotionControl,
  //kAutofocusControl,
  //kCameraControl
};

class MachineJob : public QObject
{
  Q_OBJECT
public:
  explicit MachineJob(QString job_name = "Job", QObject *parent = nullptr);

  void setRepeat(uint32_t repeat);
  uint32_t getRepeat();

  virtual bool isActive() = 0;
  virtual std::tuple<Target, QString> getNextCmd() = 0;
  virtual bool end() = 0;

signals:

private:
  QString job_name_;
  uint32_t repeat_ = 1;
  std::mutex repeat_mutex_;
  
};

#endif // MACHINEJOB_H
