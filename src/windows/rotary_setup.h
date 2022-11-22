#ifndef ROTARY_SETUP_H
#define ROTARY_SETUP_H

#include <QDialog>
#include <QButtonGroup>

namespace Ui {
class RotarySetup;
}

class RotarySetup : public QDialog
{
    Q_OBJECT

public:
    explicit RotarySetup(QWidget *parent = nullptr);
    ~RotarySetup();

    bool isRotaryMode();

    bool isMirrorMode();

    char getRotaryAxis();

    void setRotaryMode(bool is_rotary_mode);

    void setMirrorMode(bool is_mirror_mode);

    void setRotaryAxis(char rotary_axis);

    double getCircumference();

    double getRotaryScale();

    void setTravelSpeed(double travel_speed) {travel_speed_ = travel_speed;}

    void setFramingPower(double framing_power) {framing_power_ = framing_power;}

    void setControlEnable(bool control_enable);

    void setDefaultCircumference(double default_value);

private:
    Ui::RotarySetup *ui;
    void testRotary();
    void updateRotaryScale();
    void resetUI();

    bool is_rotary_mode_ = false;
    bool is_mirror_mode_ = false;
    char rotary_axis_ = 'Y';
    double rotary_scale_ = 0;//to fit gcode unit to circumference
    double circumference_ = 0;
    double mm_per_rotation_;
    double roller_diameter_;
    bool roller_type_;
    double travel_speed_;
    double framing_power_;
    bool control_enable_ = true;
    QButtonGroup *axis_group_;
    QString selected_dark_ = "background-color: rgb(80, 80, 80);border-radius: 10px; border: 3px rgb(49, 78, 119);border-style: outset;";
    QString selected_light_ = "background-color: rgb(255, 255, 255);border-radius: 10px; border: 3px rgb(163, 205, 255);border-style: outset;";
    QString unselect_dark_ = "background-color: rgb(34, 34, 34);border-radius: 10px;";
    QString unselect_light_ = "background-color: rgb(245, 245, 245);border-radius: 10px;";

Q_SIGNALS:
  void rotaryModeChanged(bool is_rotary_mode);
  void mirrorModeChanged(bool is_mirror_mode);
  void rotaryAxisChanged(char axis);
  void actionTestRotary(QRectF bbox, char rotary_axis, qreal feedrate, double framing_power);
  void updateCircumference(double circumference);
};

#endif // ROTARY_SETUP_H
