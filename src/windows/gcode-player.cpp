#include "gcode-player.h"
#include "ui_gcode-player.h"
#include <QMessageBox>

#ifndef Q_OS_IOS

#include <QTimer>
#include <QSerialPortInfo>
#include <QtMath>

#include <QDebug>

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
    // Determine the behaviour of executeBtn based on state: Start / Stop
    if (!jobs_.isEmpty() && jobs_.last()->isRunning() && jobs_.last()->status() == SerialJob::Status::RUNNING) {
      // STOP
      jobs_.last()->stop();
    } else {
      if (!jobs_.isEmpty() && jobs_.last()->isFinished()) {
        // Delete finished/stopped job first
        jobs_.pop_back();
      }
      // START
      auto job = new SerialJob(this,
                               ui->portComboBox->currentText() + ":" + ui->baudComboBox->currentText(),
                               ui->gcodeText->toPlainText().split("\n"));
      jobs_ << job;
      qRegisterMetaType<BaseJob::Status>();
      qRegisterMetaType<uint32_t>();
      connect(job, &SerialJob::error, this, &GCodePlayer::showError);
      connect(job, &SerialJob::progressChanged, this, &GCodePlayer::updateProgress);
      connect(job, &SerialJob::statusChanged, this, &GCodePlayer::onStatusChanged);
      job->start();
    }
  });

  connect(ui->pauseBtn, &QAbstractButton::clicked, [=]() {
    // Pause / Resume
    if (jobs_.isEmpty()) {
      return;
    }
    auto job = jobs_.last();
    if (job->status() == SerialJob::Status::RUNNING) {
      job->pause();
    } else {
      job->resume();
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

void GCodePlayer::onStatusChanged(BaseJob::Status new_status) {
  switch (new_status) {
    case BaseJob::Status::READY:
      break;
    case BaseJob::Status::RUNNING:
      qInfo() << "Running";
      ui->pauseBtn->setEnabled(true);
      ui->pauseBtn->setText(tr("Pause"));
      ui->executeBtn->setEnabled(true);
      ui->executeBtn->setText(tr("Stop"));
      break;
    case BaseJob::Status::PAUSED:
      qInfo() << "Paused";
      ui->pauseBtn->setEnabled(true);
      ui->pauseBtn->setText(tr("Resume"));
      break;
    case BaseJob::Status::FINISHED:
      qInfo() << "Finished";
      ui->pauseBtn->setEnabled(false);
      ui->pauseBtn->setText(tr("Pause"));
      ui->executeBtn->setText(tr("Execute"));
      ui->executeBtn->setEnabled(true);
      updateProgress();
      break;
    case BaseJob::Status::STOPPING:
    case BaseJob::Status::ERROR_STOPPING:
      qInfo() << "Stopping";
      ui->pauseBtn->setEnabled(false);
      ui->executeBtn->setEnabled(false);
      break;
    case BaseJob::Status::STOPPED:
    case BaseJob::Status::ERROR_STOPPED:
      qInfo() << "Stopped";
      ui->pauseBtn->setEnabled(false);
      ui->pauseBtn->setText(tr("Pause"));
      ui->executeBtn->setText(tr("Execute"));
      ui->executeBtn->setEnabled(true);
      updateProgress();
      break;
    default:
      break;
  }
}

void GCodePlayer::updateProgress() {
  ui->progressBarLabel->setText(QString::number(jobs_.last()->progress())+"%");
  ui->progressBar->setValue(jobs_.last()->progress());
}

void GCodePlayer::setGCode(const QString &gcode) {
  ui->gcodeText->setPlainText(gcode);
  this->calcRequiredTime(gcode);
}

void GCodePlayer::calcRequiredTime(const QString &gcode) {
  QStringList gcodeList = gcode.split('\n');
  int current_line = 0;
  float last_x = 0, last_y = 0, f = 7500, x = 0, y = 0;
  required_time_ = 0;
  while (current_line < gcodeList.size()) {
    x = last_x;
    y = last_y;

    if (gcodeList[current_line].indexOf("G1") > -1) {
      if (gcodeList[current_line].indexOf("F") > -1) {
        QRegularExpression re("F(\\d+)");
        QRegularExpressionMatch match = re.match(gcodeList[current_line]);
        if (match.hasMatch()) {
          f = match.captured(1).toFloat();
        }
      }
      if (gcodeList[current_line].indexOf("X") > -1) {
        QRegularExpression re("X(\\d+.\\d+)");
        QRegularExpressionMatch match = re.match(gcodeList[current_line]);
        if (match.hasMatch()) {
          x = match.captured(1).toFloat();
        }
      }

      if (gcodeList[current_line].indexOf("Y") > -1) {
        QRegularExpression re("Y(\\d+.\\d+)");
        QRegularExpressionMatch match = re.match(gcodeList[current_line]);
        if (match.hasMatch()) {
          y = match.captured(1).toFloat();
        }
      }

      required_time_ +=  qSqrt(qPow(x-last_x, 2) + qPow(y-last_y, 2)) / f * 60;

      last_x = x;
      last_y = y;
    }

    current_line++;
  }
}

QString GCodePlayer::requiredTime() const {
  int num = (int) required_time_;
  QString required_time_str = "";

  if (num >= 3600) {
    required_time_str += QString::number(num/3600);
    required_time_str += "h ";
    num = num % 3600;
  }

  if (num >= 60) {
    required_time_str += QString::number(num/60);
    required_time_str += "m ";
    num = num % 60;
  }

  if (num != 0) {
    required_time_str += QString::number(num);
    required_time_str += "s";
  }

  return required_time_str;
}

GCodePlayer::~GCodePlayer() {
  delete ui;
}
