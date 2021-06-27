#ifndef DOCSETTINGSPANEL_H
#define DOCSETTINGSPANEL_H

#include <QFrame>
#include <canvas/canvas.h>

namespace Ui {
  class DocPanel;
}

class DocPanel : public QFrame {
Q_OBJECT

public:
  explicit DocPanel(QWidget *parent, Canvas *canvas);

  ~DocPanel();

  void loadSettings();

  void registerEvents();

private:
  Ui::DocPanel *ui;
  Canvas *canvas_;
};

#endif // DOCSETTINGSPANEL_H
