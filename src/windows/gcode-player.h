#ifndef GCODEPLAYER_H
#define GCODEPLAYER_H

#include <QDialog>
#include <QFrame>
#include <QThread>

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

  void setSerialPort();

  void setGCode(const QString &string);

  void showError(const QString &string);

private:

  void loadSettings() override;

  void registerEvents() override;

  Ui::GCodePlayer *ui;

#ifndef Q_OS_IOS
  QList<SerialJob *> jobs_;
#endif
};

#endif // GCODEPLAYER_H
