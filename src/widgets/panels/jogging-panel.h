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

  void setAxisPosition(double x_position, double y_position);

  ~JoggingPanel();

signals:
  void panelShow(bool is_show);
  
  // Delegate action to other component
  void actionLaser(qreal power_percent);
  void actionLaserPulse(qreal power_percent);
  void actionHome();
  void actionMoveRelatively(qreal x, qreal y, qreal feedrate);
  void actionMoveToEdge(int edge_id, qreal feedrate);
  void actionMoveToCorner(int corner_id, qreal feedrate);
  
public slots:
  // Accepting signals from QML action
  void laser();
  void laserPulse();
  void home();
  void moveRelatively(int dir, int level);
  void moveToEdge(int dir);
  void moveToCorner(int corner);

  void setControlEnable(bool control_enable);

private:
  void hideEvent(QHideEvent *event) override;
  void showEvent(QShowEvent *event) override;
  
  Ui::JoggingPanel *ui;

  MainWindow *main_window_;

  bool is_laser_on_ = false;

  bool control_enable_ = true;
};

#endif // JOGGING_WIDGET_H
