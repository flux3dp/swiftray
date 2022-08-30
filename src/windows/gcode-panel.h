#pragma once

#include <QDialog>
#include <QFrame>
#include <QThread>
#include <QQuickItem>

#ifndef Q_OS_IOS

#include <widgets/base-container.h>
#include <motion_controller_job/base-job.h>

#endif

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

  void attachJob(BaseJob *job);

public slots:
  void onStatusChanged(BaseJob::Status);
  void onProgressChanged(QVariant);
  
private:

  void loadSettings() override;

  void registerEvents() override;

  void hideEvent(QHideEvent *event) override;
  
  void showEvent(QShowEvent *event) override;

  void checkGenerateGcode();

  Ui::GCodePanel *ui;

  BaseJob::Status status_;
  BaseJob *job_;

signals:
  void exportGcode();

  void importGcode();

  void generateGcode();

  void startBtnClicked();
  void stopBtnClicked();
  void pauseBtnClicked();
  void resumeBtnClicked();
  void jobStatusReport(BaseJob::Status status);
  void panelShow(bool is_show);
};
