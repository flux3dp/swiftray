#ifndef JOGGING_WIDGET_H
#define JOGGING_WIDGET_H

#include <QFrame>
#include <QDebug>
#include <widgets/base-container.h>

class MainWindow; 
namespace Ui {
  class JoggingPanel;
}

class JoggingPanel : public QFrame, BaseContainer {
Q_OBJECT

public:
  explicit JoggingPanel(QWidget *parent, MainWindow *main_window);

  void sendJob(QString &job_str);

  ~JoggingPanel();

public slots:
  void laser();

  void laserPulse();

  void home();

  void moveRelatively(int dir, int level);

  void moveToEdge(int dir);

  void moveToCorner(int corner);

  QPointF transformDirection(QPointF movement);

private:
  Ui::JoggingPanel *ui;

  MainWindow *main_window_;

  bool is_laser_on_ = false;
};

#endif // JOGGING_WIDGET_H
