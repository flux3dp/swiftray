#ifndef ROTARY_SETUP_H
#define ROTARY_SETUP_H

#include <QButtonGroup>
#include <QDialog>
#include <QList>
#include <settings/rotary-settings.h>

namespace Ui {
class RotarySetup;
}

class RotarySetup : public QDialog
{
  Q_OBJECT

public:
  struct RotarySetting {
    int rotary_index;
    bool rotary_mode;
    bool mirror_mode;
    double circumference;
    char rotary_axis;
  };
  explicit RotarySetup(RotarySetting setting, QWidget *parent = nullptr);
  ~RotarySetup();
  void setRotaryIndex(int rotary_index);
  void setRotaryMode(bool is_rotary_mode);
  void setMirrorMode(bool is_mirror_mode);
  void setCircumference(double circumference);
  void setRotaryAxis(char rotary_axis);

private:
  Ui::RotarySetup *ui;
  void testRotary();
  void updateRotaryScale();
  void resetDeviceList(const QList<RotarySettings::RotaryParam> &rotary_list);
  void updateUI(RotarySetting setting);
  void updateRotarySelect(int rotary_index);
  void updateRotaryTypeSelect(RotarySettings::RotaryType rotary_type);

  QButtonGroup *axis_group_;
  QString selected_dark_ = "background-color: rgb(80, 80, 80);border-radius: 10px; border: 3px rgb(49, 78, 119);border-style: solid;";
  QString selected_light_ = "background-color: rgb(255, 255, 255);border-radius: 10px; border: 3px rgb(163, 205, 255);border-style: solid;";
  QString unselect_dark_ = "background-color: rgb(34, 34, 34);border-radius: 10px;";
  QString unselect_light_ = "background-color: rgb(220, 220, 220);border-radius: 10px;";
  QList<RotarySettings::RotaryParam> rotary_list_;

Q_SIGNALS:
  void updateRotaryIndex(int rotary_index);
  void rotaryModeChanged(bool is_rotary_mode);
  void mirrorModeChanged(bool is_mirror_mode);
  void rotaryAxisChanged(char axis);
  void actionTestRotary(double test_distance);
  void updateCircumference(double circumference);
  void updateRotarySpeed(double travel_speed);
};

#endif // ROTARY_SETUP_H
