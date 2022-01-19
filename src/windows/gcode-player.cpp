#include "gcode-player.h"
#include "ui_gcode-player.h"
#include <QMessageBox>
#include <QRegularExpression>

#ifndef Q_OS_IOS

#include <QTimer>
#include <SerialPort/SerialPort.h>
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

void GCodePlayer::loadSettings() {}

void GCodePlayer::registerEvents() {
#ifndef Q_OS_IOS
  connect(ui->executeBtn, &QAbstractButton::clicked, this, &GCodePlayer::startBtnClicked);
  connect(ui->stopBtn, &QAbstractButton::clicked, this, &GCodePlayer::stopBtnClicked);
  connect(ui->pauseBtn, &QAbstractButton::clicked, [=]() {
    // Pause / Resume
    if (status_ == BaseJob::Status::RUNNING) {
      emit pauseBtnClicked();
    } else {
      emit resumeBtnClicked();
    }
  });
  connect(ui->exportBtn, &QAbstractButton::clicked, this, &GCodePlayer::exportGcode);
  connect(ui->importBtn, &QAbstractButton::clicked, this, &GCodePlayer::importGcode);
  connect(ui->generateBtn, &QAbstractButton::clicked, this, &GCodePlayer::generateGcode);

  connect(&(SerialPort::getInstance()), &SerialPort::connected, [=]() {
      qInfo() << "[SerialPort] Success connect!";
      ui->executeBtn->setText(tr("Execute"));
      ui->executeBtn->setEnabled(true);
  });
  connect(&(SerialPort::getInstance()), &SerialPort::disconnected, [=]() {
      qInfo() << "[SerialPort] Disconnected!";
      ui->executeBtn->setText(tr("Execute"));
      ui->executeBtn->setEnabled(false);
  });
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
  status_ = new_status;
  switch (status_) {
    case BaseJob::Status::READY:
      break;
    case BaseJob::Status::RUNNING:
      qInfo() << "Running";
      ui->pauseBtn->setEnabled(true);
      ui->pauseBtn->setText(tr("Pause"));
      ui->executeBtn->setEnabled(false);
      ui->stopBtn->setEnabled(true);
      //ui->executeBtn->setText(tr("Stop"));
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
      ui->executeBtn->setEnabled(true);
      ui->stopBtn->setEnabled(false);
      onProgressChanged(100);
      break;
    case BaseJob::Status::STOPPING:
    case BaseJob::Status::ERROR_STOPPING:
      qInfo() << "Stopping";
      ui->pauseBtn->setEnabled(false);
      ui->executeBtn->setEnabled(false);
      ui->stopBtn->setEnabled(false);
      break;
    case BaseJob::Status::STOPPED:
    case BaseJob::Status::ERROR_STOPPED:
      qInfo() << "Stopped";
      ui->pauseBtn->setEnabled(false);
      ui->pauseBtn->setText(tr("Pause"));
      ui->executeBtn->setEnabled(true);
      ui->stopBtn->setEnabled(false);
      onProgressChanged(0);
      break;
    default:
      break;
  }
}

void GCodePlayer::onProgressChanged(QVariant progress) {
  ui->progressBar->setValue(progress.toInt());
  ui->progressBarLabel->setText(ui->progressBar->text());
}

void GCodePlayer::setGCode(const QString &gcode) {
  ui->gcodeText->setPlainText(gcode);
}

QString GCodePlayer::getGCode() {
  return ui->gcodeText->toPlainText();
}

/**
 * @brief Calculate the required time from the GCodes inside text area
 * @retval A list of timestamp corresponding to each line of GCode
 */
QList<QTime> GCodePlayer::calcRequiredTime() {
  QList<QTime> timestamp_list;
  QStringList gcode_list = ui->gcodeText->toPlainText().split('\n');
  int current_line = 0;
  float last_x = 0, last_y = 0, f = 7500, x = 0, y = 0;

  QTime required_time{0, 0};
  while (current_line < gcode_list.size()) {
    x = last_x;
    y = last_y;

    if (gcode_list[current_line].indexOf("G1") > -1) {
      if (gcode_list[current_line].indexOf("F") > -1) {
        QRegularExpression re("F(\\d+)");
        QRegularExpressionMatch match = re.match(gcode_list[current_line]);
        if (match.hasMatch()) {
          f = match.captured(1).toFloat();
        }
      }
      if (gcode_list[current_line].indexOf("X") > -1) {
        QRegularExpression re("X(\\d+.\\d+)");
        QRegularExpressionMatch match = re.match(gcode_list[current_line]);
        if (match.hasMatch()) {
          x = match.captured(1).toFloat();
        }
      }

      if (gcode_list[current_line].indexOf("Y") > -1) {
        QRegularExpression re("Y(\\d+.\\d+)");
        QRegularExpressionMatch match = re.match(gcode_list[current_line]);
        if (match.hasMatch()) {
          y = match.captured(1).toFloat();
        }
      }

      // NOTE: F value is in unit of mm/min
      required_time = required_time.addMSecs(1000 *
                  qSqrt(qPow(x-last_x, 2) + qPow(y-last_y, 2)) / f * 60);

      last_x = x;
      last_y = y;
    }

    timestamp_list << required_time;
    current_line++;
  }

  return timestamp_list;
}

GCodePlayer::~GCodePlayer() {
  delete ui;
}

void GCodePlayer::attachJob(BaseJob *job) {
  job_ = job;
  //qRegisterMetaType<BaseJob::Status>(); // NOTE: This is necessary for passing enum class argument for signal/slot
  connect(job_, &BaseJob::error, this, &GCodePlayer::showError);
  connect(job_, &BaseJob::progressChanged, this, &GCodePlayer::onProgressChanged);
  connect(job_, &BaseJob::statusChanged, this, &GCodePlayer::onStatusChanged);

  onStatusChanged(job_->status());
  onProgressChanged(0);
}
