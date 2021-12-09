#ifndef JOGGING_WIDGET_H
#define JOGGING_WIDGET_H

#include <QFrame>
#include <QDebug>
#include <settings/maintenance-controller.h>
#include <widgets/base-container.h>

class MainWindow; 
namespace Ui {
  class JoggingPanel;
}

class JoggingPanel : public QFrame, BaseContainer {
Q_OBJECT

public:
  explicit JoggingPanel(QWidget *parent, MainWindow *main_window);

  ~JoggingPanel();

private:
  Ui::JoggingPanel *ui;

  MainWindow *main_window_;
};

#endif // JOGGING_WIDGET_H
