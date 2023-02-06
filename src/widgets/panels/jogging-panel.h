#ifndef JOGGING_WIDGET_H
#define JOGGING_WIDGET_H

#include <QFrame>
#include <QDebug>
#include <widgets/base-container.h>
#include <tuple>

class MainWindow; 
namespace Ui {
  class JoggingPanel;
}

class JoggingPanel : public QFrame, BaseContainer {
Q_OBJECT

public:
  explicit JoggingPanel(QWidget *parent, MainWindow *main_window);
  void setTravelSpeed(double travel_speed) {travel_speed_ = travel_speed;}
  void setFramingPower(double power);
  void setPulsePower(double power);
  bool getShowCurrent();
  bool getShowUserOrigin();

  ~JoggingPanel();

Q_SIGNALS:
  void panelShow(bool is_show);
  
  // Delegate action to other component
  void actionLaser(qreal power_percent);
  void actionLaserPulse(qreal power_percent);
  void actionHome();
  void actionMoveRelatively(qreal x, qreal y, qreal feedrate);
  void actionMoveAbsolutely(std::tuple<qreal, qreal, qreal>pos, qreal feedrate);
  void actionMoveToEdge(int edge_id, qreal feedrate);
  void actionMoveToCorner(int corner_id, qreal feedrate);
  void actionSetOrigin(std::tuple<qreal, qreal, qreal> new_origin);
  void updateFramingPower(double framing_power);
  void updatePulsePower(double pulse_power);
  void stopBtnClicked();
  void showCurrentPosition(bool show);
  void showUserOrigin(bool show);
  
public Q_SLOTS:
  // Accepting signals from QML action
  void laser();
  void laserPulse();
  void home();
  void moveRelatively(int dir, int level);
  void moveAbsolutely(std::tuple<qreal, qreal, qreal> pos);
  void moveToEdge(int dir);
  void moveToCorner(int corner);
  void updateCurrentPos(std::tuple<qreal, qreal, qreal> pos);
  void setOrigin();
  void clearOrigin();

  void setControlEnable(bool control_enable);

private:
  void hideEvent(QHideEvent *event) override;
  void showEvent(QShowEvent *event) override;
  
  Ui::JoggingPanel *ui;

  MainWindow *main_window_;

  bool is_laser_on_ = false;

  bool control_enable_ = true;

  double travel_speed_;
};

#endif // JOGGING_WIDGET_H
