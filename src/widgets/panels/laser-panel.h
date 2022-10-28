#ifndef LASERPANEL_H
#define LASERPANEL_H

#include <QFrame>
#include <widgets/base-container.h>

class MainWindow;

namespace Ui {
class LaserPanel;
}

class LaserPanel : public QFrame, BaseContainer
{
    Q_OBJECT

public:
    enum JobOrigin{
        NW = 0,
        N,
        NE,
        E,
        SE,
        S,
        SW,
        W,
        CENTER
    };
    enum StartFrom{
        AbsoluteCoords = 0,
        UserOrigin,
        CurrentPosition
    };
    Q_ENUM(StartFrom)

    explicit LaserPanel(QWidget *parent, MainWindow *main_window_);
    ~LaserPanel();
    void setJobOrigin(JobOrigin position);
    void setStartFrom(StartFrom start_from);
    int getJobOrigin();
    int getStartFrom();
    bool getStartWithHome();
    void setControlEnable(bool control_enable);
    void setStartHomeEnable(bool control_enable);

signals:
  void actionFrame();
  void actionPreview();
  void actionStart();
  void actionHome();
  void actionMoveToOrigin();
  void selectJobOrigin(JobOrigin position);
  void switchStartFrom(StartFrom start_from);
  void panelShow(bool is_show);
  void startWithHome(bool start_with_home);

private:
    void loadStyles() override;
    void registerEvents() override;
    void setLayout();
    void hideEvent(QHideEvent *event) override;
    void showEvent(QShowEvent *event) override;
    Ui::LaserPanel *ui;
    JobOrigin job_origin_ = NW;
    StartFrom start_from_ = AbsoluteCoords;
    MainWindow *main_window_;
    bool start_with_home_ = true;
};

#endif // LASERPANEL_H
