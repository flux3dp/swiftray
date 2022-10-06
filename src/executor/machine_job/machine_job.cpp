#include "machine_job.h"

#include <QtMath>
#include <QDebug>

MachineJob::MachineJob(QString job_name)
{
  job_name_ = job_name;
}

bool MachineJob::withPreview() const {
  return with_preview_;
}

QPixmap MachineJob::getPreview() const {
  return preview_;
}

/**
 * @brief Calculate the required time from the GCodes inside text area
 * @retval A list of timestamp corresponding to each line of GCode
 *         throw exception when canceled
 */
QList<Timestamp> MachineJob::calcRequiredTime(const QStringList &gcode_list, 
                                        QPointer<QProgressDialog> progress_dialog) {
  QList<Timestamp> timestamp_list;
  if (!progress_dialog.isNull()) {
    progress_dialog->setMaximum(gcode_list.size()-1);
  }
  //QStringList gcode_list = ui->gcodeText->toPlainText().split('\n');
  int current_line = 0;
  //int g_motion_modal = 0;    // 0, 1, 2, 3, 80, 81, 82, 84, 85, 86, 87, 88, 89
  //int g_distance_modal = 90; // 90, 91
  bool relative_mode = false; // G90 or G91
  float last_abs_x = 0, last_abs_y = 0, last_abs_z = 0;
  float x_param = 0, y_param = 0, z_param = 0, f_param = 7500;

  Timestamp required_time;

  bool canceled = false;
  if (!progress_dialog.isNull()) {
    connect(progress_dialog, &QProgressDialog::canceled, [&]() {
        canceled = true;
    });
    progress_dialog->setWindowModality(Qt::WindowModal);
    //progress.setWindowModality(Qt::NonModal);
    //progress.setWindowModality(Qt::ApplicationModal);
    progress_dialog->show();
  }
  while (current_line < gcode_list.size()) {
    if (canceled) {
      throw "Canceled";
    }
    if (current_line % 100 == 0 || current_line == (gcode_list.size() - 1)) {
      if (!progress_dialog.isNull()) {
        progress_dialog->setValue(current_line);
      }
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
            // ignore M-code
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

/**
 * @brief Calculate the required time from the GCodes inside text area
 * @retval A list of timestamp corresponding to each line of GCode
 *         throw exception when canceled
 */
QList<Timestamp> MachineJob::calcRequiredTime(QStringList &&gcode_list, 
                                        QPointer<QProgressDialog> progress_dialog) {
  QList<Timestamp> timestamp_list;
  if (!progress_dialog.isNull()) {
    progress_dialog->setMaximum(gcode_list.size()-1);
  }
  //QStringList gcode_list = ui->gcodeText->toPlainText().split('\n');
  int current_line = 0;
  //int g_motion_modal = 0;    // 0, 1, 2, 3, 80, 81, 82, 84, 85, 86, 87, 88, 89
  //int g_distance_modal = 90; // 90, 91
  bool relative_mode = false; // G90 or G91
  float last_abs_x = 0, last_abs_y = 0, last_abs_z = 0;
  float x_param = 0, y_param = 0, z_param = 0, f_param = 7500;

  Timestamp required_time;

  bool canceled = false;
  if (!progress_dialog.isNull()) {
    connect(progress_dialog, &QProgressDialog::canceled, [&]() {
        canceled = true;
    });
    progress_dialog->setWindowModality(Qt::WindowModal);
    //progress.setWindowModality(Qt::NonModal);
    //progress.setWindowModality(Qt::ApplicationModal);
    progress_dialog->show();
  }
  while (current_line < gcode_list.size()) {
    if (canceled) {
      throw "Canceled";
    }
    if (current_line % 100 == 0 || current_line == (gcode_list.size() - 1)) {
      if (!progress_dialog.isNull()) {
        progress_dialog->setValue(current_line);
      }
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
            // ignore M-code
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
