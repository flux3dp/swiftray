#ifndef GCODEPLAYER_H
#define GCODEPLAYER_H

#include <QDialog>
#include <QFrame>
#include <QThread>

#ifndef Q_OS_IOS

#include <QSerialPort>
#include <connection/serialport-thread.h>

#endif

namespace Ui {
  class GCodePlayer;
}

class GCodePlayer : public QFrame {
Q_OBJECT

public:
  explicit GCodePlayer(QWidget *parent = nullptr);

  ~GCodePlayer();

  void loadSettings();

  void registerEvents();

  void setSerialPort();

  void setGCode(const QString &string);

  void showError(const QString &string);

private:
  Ui::GCodePlayer *ui;

#ifndef Q_OS_IOS
  SerialPortThread thread_;
#endif
};

#endif // GCODEPLAYER_H
