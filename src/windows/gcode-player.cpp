#include "gcode-player.h"
#include "ui_gcode-player.h"
#include <QMessageBox>

#ifndef Q_OS_IOS

#include <QTimer>
#include <QSerialPortInfo>
#include <QtMath>

#endif

GCodePlayer *gcode_player_ = nullptr;

GCodePlayer::GCodePlayer(QWidget *parent) :
     QFrame(parent),
     ui(new Ui::GCodePlayer),
     BaseContainer() {
  ui->setupUi(this);
  gcode_player_ = this;
  initializeContainer();
}

void GCodePlayer::loadSettings() {
#ifndef Q_OS_IOS
  const auto infos = QSerialPortInfo::availablePorts();
  ui->portComboBox->clear();
  for (const QSerialPortInfo &info : infos)
    ui->portComboBox->addItem(info.portName());
  ui->portComboBox->setCurrentIndex(ui->portComboBox->count() - 1);
  qmlRegisterType<MaintenanceController>("MaintenanceController", 1, 0, "MaintenanceController");
  ui->maintenanceController->setSource(QUrl("qrc:/src/windows/qml/MaintenanceDialog.qml"));
  ui->maintenanceController->show();
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

QString GCodePlayer::portName() {
  return gcode_player_->ui->portComboBox->currentText();
}

QString GCodePlayer::baudRate() {
  return gcode_player_->ui->baudComboBox->currentText();
}

GCodePlayer::~GCodePlayer() {
  gcode_player_ = nullptr;
  delete ui;
}
