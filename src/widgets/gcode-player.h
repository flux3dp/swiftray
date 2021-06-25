#ifndef GCODEPLAYER_H
#define GCODEPLAYER_H

#include <QDialog>
#include <QFrame>

namespace Ui {
  class GCodePlayer;
}

class GCodePlayer : public QFrame {
Q_OBJECT

public:
  explicit GCodePlayer(QWidget *parent = nullptr);

  ~GCodePlayer();

  void loadSettings();

  void setGCode(const QString &string);

private:
  Ui::GCodePlayer *ui;
};

#endif // GCODEPLAYER_H
