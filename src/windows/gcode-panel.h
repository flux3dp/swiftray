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

namespace Ui {
  class GCodePanel;
}

class GCodePanel : public QFrame, BaseContainer {
Q_OBJECT

public:

  explicit GCodePanel(QWidget *parent = nullptr);

  ~GCodePanel();

  void setGCode(const QString &gcodes);
  QString getGCode();

  void showError(const QString &string);

  void attachJob(QPointer<JobExecutor> job_executor);

public slots:
  void onJobStateChanged(Executor::State);
  void onJobProgressChanged(QVariant);
  
private:

  void loadSettings() override;

  void registerEvents() override;

  void hideEvent(QHideEvent *event) override;
  
  void showEvent(QShowEvent *event) override;

  void checkGenerateGcode();

  Ui::GCodePanel *ui;

  Executor::State job_state_;

  QPointer<JobExecutor> job_executor_;

signals:
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
