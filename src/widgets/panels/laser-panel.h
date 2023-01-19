#ifndef LASERPANEL_H
#define LASERPANEL_H

#include <QFrame>
#include <widgets/base-container.h>

namespace Ui {
class LaserPanel;
}

class LaserPanel : public QFrame, BaseContainer
{
  Q_OBJECT

public:
  explicit LaserPanel(QWidget *parent, bool is_dark_mode);
  ~LaserPanel();
  void setJobOrigin(int position);
  void setStartFrom(int start_from);
  void setStartHome(bool find_home);
  bool getStartHome();
  void setStartHomeEnable(bool enable);
  void setControlEnable(bool enable);

Q_SIGNALS:
  void actionFrame();
  void actionPreview();
  void actionStart();
  void actionHome();
  void actionMoveToOrigin();
  void selectJobOrigin(int position);
  void switchStartFrom(int start_from);
  void panelShow(bool is_show);
  void startWithHome(bool start_with_home);

private:
  void loadStyles() override;
  void registerEvents() override;
  void setLayout(bool is_dark_mode);
  void hideEvent(QHideEvent *event) override;
  void showEvent(QShowEvent *event) override;
  Ui::LaserPanel *ui;
};

#endif // LASERPANEL_H
