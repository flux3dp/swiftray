#ifndef DOCSETTINGSPANEL_H
#define DOCSETTINGSPANEL_H

#include <QFrame>

class MainWindow;

namespace Ui {
  class DocPanel;
}

class DocPanel : public QFrame {
Q_OBJECT

public:
  explicit DocPanel(QWidget *parent, MainWindow *main_window);

  ~DocPanel();

  void loadSettings();

  void registerEvents();

private:
  Ui::DocPanel *ui;
  MainWindow *main_window_;
};

#endif // DOCSETTINGSPANEL_H
