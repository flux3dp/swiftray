#pragma once

#include <QDialog>
#include <QFrame>
#include <QThread>
#include <QQuickItem>

#ifndef Q_OS_IOS

#include <QSerialPort>
#include <connection/serial-job.h>
#include <widgets/base-container.h>

#endif

namespace Ui {
  class GCodePlayer;
}

class GCodePlayer : public QFrame, BaseContainer {
Q_OBJECT

public:

  explicit GCodePlayer(QWidget *parent = nullptr);

  ~GCodePlayer();

  void calcRequiredTime(const QString &string);

  QString requiredTime() const;

  void setSerialPort();

  void setGCode(const QString &string);

  void showError(const QString &string);

  void updateProgress();

private slots:
  void onStatusChanged(BaseJob::Status new_status);

private:

  void loadSettings() override;

  void registerEvents() override;

  Ui::GCodePlayer *ui;

  float required_time_;

#ifndef Q_OS_IOS
  QList<SerialJob *> jobs_;
#endif
};
