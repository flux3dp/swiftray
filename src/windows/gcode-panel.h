#pragma once

#include <QDialog>
#include <QFrame>
#include <QThread>
#include <QQuickItem>
#include <QPointer>

#include <widgets/base-container.h>
#include <executor/machine_job/machine_job.h>
#include <executor/executor.h>
#include <executor/job_executor.h>

class MainWindow;

namespace Ui {
  class GCodePanel;
}

class GCodePanel : public QFrame, BaseContainer {
Q_OBJECT

public:

  explicit GCodePanel(QWidget *parent, MainWindow *main_window);

  ~GCodePanel();

  void setGCode(const QString &gcodes);
  QString getGCode();

  void attachJob(QPointer<JobExecutor> job_executor);

public SLOTS:
  void onJobStateChanged(Executor::State);
  void onJobProgressChanged(QVariant);

  void onEnableJobCtrl();
  void onDisableJobCtrl();
  
private:

  void loadSettings() override;

  void registerEvents() override;

  void hideEvent(QHideEvent *event) override;
  
  void showEvent(QShowEvent *event) override;

  void checkGenerateGcode();

  Ui::GCodePanel *ui;

  Executor::State job_state_;
  bool enabled_ = false;

  QPointer<JobExecutor> job_executor_;
  QPointer<MainWindow> main_window_;

SIGNALS:
  void exportGcode();

  void importGcode();

  void generateGcode();

  void startBtnClicked();
  void stopBtnClicked();
  void pauseBtnClicked();
  void resumeBtnClicked();
  void jobStatusReport(Executor::State state);
  void panelShow(bool is_show);
};
