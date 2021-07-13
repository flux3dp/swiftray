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
    auto job = new SerialJob(this,
                             ui->portComboBox->currentText() + ":" + ui->baudComboBox->currentText(),
                             ui->gcodeText->toPlainText().split("\n"));
    jobs_ << job;
    connect(job, &SerialJob::error, this, &GCodePlayer::showError);
    job->start();
    ui->pauseBtn->setEnabled(true);
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
}

void GCodePlayer::setGCode(const QString &gcode) {
  ui->gcodeText->setPlainText(gcode);
}

GCodePlayer::~GCodePlayer() {
  delete ui;
}
