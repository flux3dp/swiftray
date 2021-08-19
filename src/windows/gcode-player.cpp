#include "gcode-player.h"
#include "ui_gcode-player.h"
#include <QMessageBox>

#ifndef Q_OS_IOS

#include <QTimer>
#include <QSerialPortInfo>

#endif

GCodePlayer::GCodePlayer(QWidget *parent) :
     QFrame(parent),
     ui(new Ui::GCodePlayer),
     BaseContainer() {
  ui->setupUi(this);
  initializeContainer();
}

void GCodePlayer::loadSettings() {
#ifndef Q_OS_IOS
  const auto infos = QSerialPortInfo::availablePorts();
  ui->portComboBox->clear();
  for (const QSerialPortInfo &info : infos)
    ui->portComboBox->addItem(info.portName());
  ui->portComboBox->setCurrentIndex(ui->portComboBox->count() - 1);
#endif
}

void GCodePlayer::registerEvents() {
#ifndef Q_OS_IOS
  connect(ui->executeBtn, &QAbstractButton::clicked, [=]() {
    if (!jobs_.isEmpty() && jobs_.last()->status() == SerialJob::Status::RUNNING) {
      jobs_.last()->stop();
      ui->pauseBtn->setEnabled(false);
      ui->executeBtn->setText(tr("Execute"));
    } else {
      auto job = new SerialJob(this,
                              ui->portComboBox->currentText() + ":" + ui->baudComboBox->currentText(),
                              ui->gcodeText->toPlainText().split("\n"));
      jobs_ << job;
      connect(job, &SerialJob::error, this, &GCodePlayer::showError);
      connect(job, &SerialJob::progressChanged, this, &GCodePlayer::updateProgress);
      job->start();
      ui->pauseBtn->setEnabled(true);
      ui->executeBtn->setText(tr("Stop"));
    }
  });

  connect(ui->pauseBtn, &QAbstractButton::clicked, [=]() {
    auto job = jobs_.last();
    if (job->status() == SerialJob::Status::RUNNING) {
      job->pause();
      ui->pauseBtn->setText(tr("Resume"));
    } else {
      job->resume();
      ui->pauseBtn->setText(tr("Pause"));
    }
  });

  QTimer *timer = new QTimer(this);
  connect(timer, &QTimer::timeout, this, &GCodePlayer::loadSettings);
  timer->start(3000);
#endif
}

void GCodePlayer::showError(const QString &msg) {
  QMessageBox msgbox;
  msgbox.setText("Serial Port Error");
  msgbox.setInformativeText(msg);
  msgbox.exec();
  ui->pauseBtn->setEnabled(false);
  ui->executeBtn->setText(tr("Execute"));
  ui->pauseBtn->setText(tr("Pause"));
}

void GCodePlayer::updateProgress() {
  ui->progressBarLabel->setText(QString::number(jobs_.last()->progress())+"%");
  ui->progressBar->setValue(jobs_.last()->progress());
}

void GCodePlayer::setGCode(const QString &gcode) {
  ui->gcodeText->setPlainText(gcode);
}

GCodePlayer::~GCodePlayer() {
  delete ui;
}
