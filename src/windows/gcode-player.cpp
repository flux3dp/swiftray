#include "gcode-player.h"
#include "ui_gcode-player.h"
#include <QMessageBox>
#include <QRegularExpression>
#include <QProgressDialog>

#ifndef Q_OS_IOS

#include <QTimer>
#include <connection/serial-port.h>
#include <QtMath>
#include <globals.h>

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
  connect(ui->gcodeText, &QPlainTextEdit::textChanged, this, &GCodePlayer::checkGenerateGcode);

  connect(&serial_port, &SerialPort::connected, [=]() {
      qInfo() << "[SerialPort] Success connect!";
      ui->playBtn->setText(tr("Play"));
      if(!ui->gcodeText->toPlainText().isEmpty()) {
        ui->playBtn->setEnabled(true);
      }
  });
  connect(&serial_port, &SerialPort::disconnected, [=]() {
      qInfo() << "[SerialPort] Disconnected!";
      ui->playBtn->setText(tr("Play"));
      ui->playBtn->setEnabled(false);
  });
#endif
}

void GCodePlayer::checkGenerateGcode() {
  if(ui->gcodeText->toPlainText().isEmpty()) {
    ui->playBtn->setEnabled(false);
  }
  else if(serial_port.isOpen()) {
    ui->playBtn->setEnabled(true);
  }
}

void GCodePlayer::showError(const QString &msg) {
  QMessageBox msgbox;
  msgbox.setText("Job Error");
  msgbox.setInformativeText(msg);
  msgbox.exec();
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
  emit jobStatusReport(new_status);
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
 *         throw exception when canceled
 */
QList<QTime> GCodePlayer::calcRequiredTime() {
  QList<QTime> timestamp_list;
  QStringList gcode_list = ui->gcodeText->toPlainText().split('\n');
  int current_line = 0;
  //int g_motion_modal = 0;    // 0, 1, 2, 3, 80, 81, 82, 84, 85, 86, 87, 88, 89
  //int g_distance_modal = 90; // 90, 91
  bool relative_mode = false; // G90 or G91
  float last_abs_x = 0, last_abs_y = 0, last_abs_z = 0;
  float x_param = 0, y_param = 0, z_param = 0, f_param = 7500;

  QTime required_time{0, 0};

  bool canceled = false;
  QProgressDialog progress(tr("Estimating task time..."),  tr("Cancel"), 0, gcode_list.size()-1, this);
  connect(&progress, &QProgressDialog::canceled, [&]() {
      canceled = true;
  });
  progress.setWindowModality(Qt::WindowModal);
  progress.show();
  while (current_line < gcode_list.size()) {
    if (canceled) {
      throw "Canceled";
    }
    if (current_line % 100 == 0 || current_line == (gcode_list.size() - 1)) {
      progress.setValue(current_line);
    }
    QString line = gcode_list[current_line];
    line = line.toUpper().section(';', 0, 0); // Eliminate comment (;)
    if (line.startsWith("B", Qt::CaseSensitivity::CaseInsensitive) ||
        line.startsWith("D", Qt::CaseSensitivity::CaseInsensitive) ||
        line.startsWith("$", Qt::CaseSensitivity::CaseInsensitive)) {
      // do nothing (FLUX's custom cmd)
    } else {
      QChar current_param{';'};
      QString val_str;
      line.append(' '); // To ensure the last param is processed
      // Set default value when X/Y field is absent
      x_param = relative_mode ? 0 : last_abs_x;
      y_param = relative_mode ? 0 : last_abs_y;
      z_param = relative_mode ? 0 : last_abs_z;
      for (auto& c: line) {
        if (c == '-' || c == '.' || c.isDigit()) {
          if (current_param != ';') {
            val_str.append(c);
          }
        } else {
          // Finish a cmd (G-Code or M-Code)
          if (current_param == 'G') {
            if (val_str.toInt() == 90 || val_str.toInt() == 91) {
              relative_mode = val_str.toInt() == 90 ? false : true;
              // Reset default value when X/Y field is absent
              x_param = relative_mode ? 0 : last_abs_x;
              y_param = relative_mode ? 0 : last_abs_y;
              z_param = relative_mode ? 0 : last_abs_z;
            } else if (val_str.toInt() == 1) {
              // G1 Motion modal group
            }
          } else if (current_param == 'M') {
            // ignore
          }

          // Finish a param
          if (current_param == 'X') {
            x_param = val_str.toFloat();
          } else if (current_param == 'Y') {
            y_param = val_str.toFloat();
          } else if (current_param == 'Z') {
            z_param = val_str.toFloat();
          } else if (current_param == 'F') {
            f_param = val_str.toFloat();
          } else if (current_param == 'S') {
            // ignore
          }

          // The start of new param
          if (c == 'G' || c == 'M' || c == 'X' || c == 'Y' || c == 'Z' ||
              c == 'S' || c == 'F' ) {
            // Finish a param
            current_param = c;
            val_str.clear();
          } else {
            current_param = ';';
            val_str.clear();
          }
        }
      }

      /* GCode Analyze with Regex (time consuming)
      QRegularExpression re("((?<cmd>[GM])(?<cmd_idx>[\\d]+))?(?<param>([\\t ]*[XYZSEF][\\d.-]+)+)");
      QRegularExpressionMatch match = re.match(line);
      if ( ! match.captured("cmd").isEmpty() && match.captured("cmd")[0] == 'G') {
        if (match.captured("cmd_idx").toInt() >= 0 && match.captured("cmd_idx").toInt() <=3 ) {
          // G Motion modal group
        } else if (match.captured("cmd_idx").toInt() >= 90 && match.captured("cmd_idx").toInt() <= 91 ) {
          // G Distance modal group
        }
      }
      if ( ! match.captured("param").isEmpty()) {
        QRegularExpression param_re("[XYZSEF][\\d.-]+");
        QRegularExpressionMatch param_match = param_re.match(match.captured("param"));
      }
      */

      Q_ASSERT_X(f_param > 0, "GCode Player", "Feedrate must be larger than 0");
      // NOTE: F value is in unit of mm/min
      if (relative_mode) {
        required_time = required_time.addMSecs(
        1000 *
            qSqrt(qPow(x_param, 2) + qPow(y_param, 2) + qPow(z_param, 2)) /
            f_param *
            60);
        last_abs_x += x_param;
        last_abs_y += y_param;
        last_abs_z += z_param;
      } else {
        required_time = required_time.addMSecs(
            1000 *
            qSqrt(qPow(x_param-last_abs_x, 2) +
                    qPow(y_param-last_abs_y, 2) +
                    qPow(z_param-last_abs_z, 2)) /
            f_param *
            60);
        last_abs_x = x_param;
        last_abs_y = y_param;
        last_abs_z = z_param;
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
