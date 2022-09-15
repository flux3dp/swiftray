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
    explicit LaserPanel(QWidget *parent, MainWindow *main_window_);
    ~LaserPanel();
    void setJobOrigin(JobOrigin position);
    int getJobOrigin();

signals:
  void actionFrame();
  void actionPreview();
  void actionStart();
  void actionHome();
  void selectJobOrigin(JobOrigin position);
  void panelShow(bool is_show);

private:
    void loadStyles() override;
    void registerEvents() override;
    void setLayout();
    void hideEvent(QHideEvent *event) override;
    void showEvent(QShowEvent *event) override;
    Ui::LaserPanel *ui;
    JobOrigin job_origin_ = NW;
    MainWindow *main_window_;
};

#endif // LASERPANEL_H
