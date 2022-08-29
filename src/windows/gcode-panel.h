#pragma once

#include <QDialog>
#include <QFrame>
#include <QThread>
#include <QQuickItem>

#ifndef Q_OS_IOS

#include <widgets/base-container.h>

#endif

namespace Ui {
  class GCodePlayer;
}

class GCodePlayer : public QFrame, BaseContainer {
Q_OBJECT

public:

  explicit GCodePlayer(QWidget *parent = nullptr);

  ~GCodePlayer();

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

  Ui::GCodePlayer *ui;

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
