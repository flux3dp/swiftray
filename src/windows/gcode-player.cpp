#include "gcode-player.h"
#include "ui_gcode-player.h"
#include <QMessageBox>
#include <QRegularExpression>

#ifndef Q_OS_IOS

#include <QTimer>
#include <connection/serial-port.h>
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
  connect(ui->playBtn, &QAbstractButton::clicked, this, &GCodePlayer::startBtnClicked);
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
      ui->playBtn->setText(tr("Play"));
      ui->playBtn->setEnabled(true);
  });
  connect(&(SerialPort::getInstance()), &SerialPort::disconnected, [=]() {
      qInfo() << "[SerialPort] Disconnected!";
      ui->playBtn->setText(tr("Play"));
      ui->playBtn->setEnabled(false);
  });
#endif
}

void GCodePlayer::showError(const QString &msg) {
  QMessageBox msgbox;
  msgbox.setText("Serial Port Error");
  msgbox.setInformativeText(msg);
  msgbox.exec();
  ui->pauseBtn->setEnabled(false);
  ui->playBtn->setText(tr("Play"));
  ui->pauseBtn->setText(tr("Pause"));
}

void GCodePlayer::onStatusChanged(BaseJob::Status new_status) {
  status_ = new_status;
  switch (status_) {
    case BaseJob::Status::READY:
      ui->pauseBtn->setEnabled(false);
      ui->playBtn->setEnabled(true);
      ui->stopBtn->setEnabled(false);
      break;
    case BaseJob::Status::STARTING:
      ui->pauseBtn->setEnabled(false);
      ui->playBtn->setEnabled(false);
      ui->stopBtn->setEnabled(false);
      break;
    case BaseJob::Status::RUNNING:
      qInfo() << "Running";
      ui->pauseBtn->setEnabled(true);
      ui->pauseBtn->setText(tr("Pause"));
      ui->playBtn->setEnabled(false);
      ui->stopBtn->setEnabled(true);
      //ui->playBtn->setText(tr("Stop"));
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
      ui->playBtn->setEnabled(true);
      ui->stopBtn->setEnabled(false);
      onProgressChanged(100);
      break;
    case BaseJob::Status::ALARM:
      ui->pauseBtn->setEnabled(false);
      ui->playBtn->setEnabled(false);
      ui->stopBtn->setEnabled(false);
      onProgressChanged(0);
      break;
    case BaseJob::Status::STOPPED:
    case BaseJob::Status::ALARM_STOPPED:
      qInfo() << "Stopped";
      ui->pauseBtn->setEnabled(false);
      ui->pauseBtn->setText(tr("Pause"));
      ui->playBtn->setEnabled(true);
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
  bool relative_mode = false; // G90 or G91
  int current_line = 0;
  float last_x = 0, last_y = 0, f = 7500, x = 0, y = 0;

  QTime required_time{0, 0};
  while (current_line < gcode_list.size()) {
    const QString& line = gcode_list[current_line];
    if (line.startsWith("B", Qt::CaseSensitivity::CaseInsensitive)) {
      // do nothing (FLUX's custom cmd)
    } else if (line.startsWith("D", Qt::CaseSensitivity::CaseInsensitive)) {
      // do nothing (FLUX's custom cmd)
    } else if (line.startsWith("$", Qt::CaseSensitivity::CaseInsensitive)) {
      // do nothing (grbl's system cmd)
    } else if (line.startsWith("M", Qt::CaseSensitivity::CaseInsensitive)) {
      // do nothing (M code)
    } else {
      if (line.indexOf("G91", 0, Qt::CaseSensitivity::CaseInsensitive) > -1) {
        relative_mode = true;
      } else if (line.indexOf("G90", 0, Qt::CaseSensitivity::CaseInsensitive) > -1) {
        relative_mode = false;
      }

      // default value when X/Y field is absent
      x = relative_mode ? 0 : last_x;
      y = relative_mode ? 0 : last_y;

      if (line.indexOf("F", 0, Qt::CaseSensitivity::CaseInsensitive) > -1) {
        QRegularExpression re("[Ff]([0-9]+([.][0-9]*)?|[.][0-9]+)");
        QRegularExpressionMatch match = re.match(line);
        if (match.hasMatch()) {
          f = match.captured(1).toFloat();
        }
      }
      if (line.indexOf("X", 0, Qt::CaseSensitivity::CaseInsensitive) > -1) {
        QRegularExpression re("[Xx](-?[0-9]+([.][0-9]*)?|[.][0-9]+)");
        QRegularExpressionMatch match = re.match(line);
        if (match.hasMatch()) {
          x = match.captured(1).toFloat();
        }
      }
      if (line.indexOf("Y", 0, Qt::CaseSensitivity::CaseInsensitive) > -1) {
        QRegularExpression re("[Yy](-?[0-9]+([.][0-9]*)?|[.][0-9]+)");
        QRegularExpressionMatch match = re.match(line);
        if (match.hasMatch()) {
          y = match.captured(1).toFloat();
        }
      }

      // NOTE: F value is in unit of mm/min
      if (relative_mode) {
        required_time = required_time.addMSecs(1000 *
                                               qSqrt(qPow(x, 2) + qPow(y, 2)) / f * 60);
        last_x += x;
        last_y += y;
      } else {
        required_time = required_time.addMSecs(1000 *
                                               qSqrt(qPow(x-last_x, 2) + qPow(y-last_y, 2)) / f * 60);
        last_x = x;
        last_y = y;
      }
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
  qRegisterMetaType<BaseJob::Status>(); // NOTE: This is necessary for passing custom type argument for signal/slot
  connect(job_, &BaseJob::error, this, &GCodePlayer::showError);
  connect(job_, &BaseJob::progressChanged, this, &GCodePlayer::onProgressChanged);
  connect(job_, &BaseJob::statusChanged, this, &GCodePlayer::onStatusChanged);

  onStatusChanged(job_->status());
  onProgressChanged(0);
}
