#include "machine_job.h"
#include "constants.h"

#include <QtMath>
#include <QCoreApplication>
#include <QDebug>
#include <executor/operation_cmd/gcode_cmd.h>

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

void MachineJob::setMotionController(QPointer<MotionController> motion_controller) {
  motion_controller_ = motion_controller;
}

/**
 * Modified from MachineJob::calcRequiredTime for Promark
 * @brief Calculate the required time
 * @retval Total time required in ms
 */
double MachineJob::calcTotalTime(const QStringList& gcode_list) {
  cancelled = false;

  double total_time = 0; // ms
  double move_distance = 0;
  int current_line = 0;
  bool relative_mode = false;  // G90 or G91
  float last_abs_x = 0, last_abs_y = 0, last_abs_a = 0;
  float x_param = 0, y_param = 0, z_param = 0, a_param = 0, f_param = 7500, s_param = 0;
  int dotting_time = 0; // us
  bool hasEnd = false;
  double wobble_k = 1;
  int batch_size = qMax(1000, gcode_list.size() / 30);

  while (current_line < gcode_list.size() && !hasEnd && !cancelled) {
    QString line = gcode_list[current_line];
    line = line.toUpper();
    if (line.startsWith(";DOT", Qt::CaseSensitivity::CaseInsensitive)) {
      // Specific comment for High Speed Mode
      int dots = line.mid(4).toInt();
      total_time += dots * PromarkJobConfig::JUMP_DELAY_MS; // Jump delay for each dot
      total_time += 0.001 * dots * dotting_time; // Actual dotting time
    } else if (line.startsWith(";JUMP", Qt::CaseSensitivity::CaseInsensitive)) {
      // Specific comment for High Speed Mode
      int jumps = line.mid(5).toInt();
      total_time += jumps * PromarkJobConfig::JUMP_DELAY_MS;  // Jump delay for blank parts
    } else if (line.startsWith(";WOBBLE K", Qt::CaseSensitivity::CaseInsensitive)) {
      // Specific comment for Wobble
      wobble_k = line.mid(9).toFloat();
    } else if (line.startsWith(";", Qt::CaseSensitivity::CaseInsensitive) ||
               line.startsWith("B", Qt::CaseSensitivity::CaseInsensitive) ||
               line.startsWith("D", Qt::CaseSensitivity::CaseInsensitive) ||
               line.startsWith("$", Qt::CaseSensitivity::CaseInsensitive)) {
      // do nothing (FLUX's custom cmd)
    } else {
      QChar current_param{';'};
      QString val_str;
      line.append(' ');  // To ensure the last param is processed
      // Set default value when X/Y field is absent
      x_param = relative_mode ? 0 : last_abs_x;
      y_param = relative_mode ? 0 : last_abs_y;
      z_param = 0;
      for (auto& c : line) {
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
              z_param = 0;
            } else if (val_str.toInt() == 1) {
              // G1 Motion modal group
            }
          } else if (current_param == 'M') {
            if (val_str.toInt() == 2) {
              // End of program
              hasEnd = true;
              break;
            }
          }

          // Finish a param
          if (current_param == 'X') {
            x_param = val_str.toFloat();
          } else if (current_param == 'Y') {
            y_param = val_str.toFloat();
          } else if (current_param == 'Z') {
            z_param = val_str.toFloat();
          } else if (current_param == 'A') {
            a_param = val_str.toFloat();
          } else if (current_param == 'F') {
            f_param = val_str.toFloat();
          } else if (current_param == 'S') {
            s_param = val_str.toFloat();
          } else if (current_param == 'T') {
            dotting_time = val_str.toInt();
          }

          // The start of new param
          if (c == 'G' || c == 'M' || c == 'X' || c == 'Y' || c == 'Z' ||
              c == 'S' || c == 'F' || c == 'T' || c == 'A') {
            // Finish a param
            current_param = c;
            val_str.clear();
          } else {
            current_param = ';';
            val_str.clear();
          }
        }
      }

      if (z_param != 0) {
        // Note: Ingore acc time
        total_time += fabs(z_param) * PromarkJobConfig::Z_MS_PER_MM;
      } else {
        if (a_param != last_abs_a) {
          total_time += fabs(a_param - last_abs_a) * PromarkJobConfig::A_MS_PER_MM;
          last_abs_a = a_param;
        }
        if (relative_mode) {
          move_distance = qSqrt(qPow(x_param, 2) + qPow(y_param, 2));
          last_abs_x += x_param;
          last_abs_y += y_param;
        } else {
          move_distance = qSqrt(qPow(x_param - last_abs_x, 2) +
                                qPow(y_param - last_abs_y, 2));
          last_abs_x = x_param;
          last_abs_y = y_param;
        }
        if (move_distance > 0) {
          if (s_param == 0) {
            // jump & delay
            total_time += 1000.0 * move_distance / PromarkJobConfig::JUMP_SPEED + PromarkJobConfig::JUMP_DELAY_MS;
          } else if (dotting_time == 0) {
            // mark & delay
            total_time += 1000.0 * move_distance * wobble_k / f_param * 60 + PromarkJobConfig::LASER_DELAY_MS;
          } else {
            // jump for dotting
            total_time += 1000.0 * move_distance / PromarkJobConfig::JUMP_SPEED;
          }
        }
      }
    }
    current_line++;
    if (current_line % batch_size == 0) {
      // Use float calculation to avoid integer overflow on large gcode list
      Q_EMIT progressChanged(100.0 * current_line / gcode_list.size());
      QCoreApplication::processEvents();
    }
  }
  return total_time;
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
    if (current_line % 1000 == 0 || current_line == (gcode_list.size() - 1)) {
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


std::shared_ptr<OperationCmd> MachineJob::getNextCmd() {
  if (end()) {
    return std::shared_ptr<OperationCmd>{nullptr};
  }
  auto idx = next_gcode_idx_++;
  auto cmd = std::make_shared<GCodeCmd>(gcode_list_.at(idx) + "\n");
  return cmd;
}

QString MachineJob::getNextCmdString() {
  if (end()) return QString();
  auto idx = next_gcode_idx_++;
  return gcode_list_.at(idx);
}

float MachineJob::getProgressPercent() const {
  if (gcode_list_.isEmpty()) {
    return 100;
  } else if (next_gcode_idx_ >= gcode_list_.size()) {
    return 100;
  }
  return 100 * (float)(next_gcode_idx_) / gcode_list_.size();
}

void MachineJob::reload() {
  this->next_gcode_idx_ = 0;
}

bool MachineJob::end() const {
  return this->next_gcode_idx_ >= this->gcode_list_.size();
}

Timestamp MachineJob::getElapsedTime() const {
  return Timestamp();
}

Timestamp MachineJob::getTotalRequiredTime() const {
  return Timestamp();
}

Timestamp MachineJob::getRemainingTime() const {
  return Timestamp();
}
