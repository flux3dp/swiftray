#ifndef GCODEPLAYER_H
#define GCODEPLAYER_H

#include <QDialog>
#include <QFrame>
#include <QSerialPort>
#include <QThread>
#include <connection/serialport-thread.h>

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
  SerialPortThread thread_;
};

#endif // GCODEPLAYER_H
