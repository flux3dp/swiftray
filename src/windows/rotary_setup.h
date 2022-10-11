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

private:
    Ui::RotarySetup *ui;
    void testRotary();
    void updateRotaryScale();

    bool is_rotary_mode_ = false;
    bool is_mirror_mode_ = false;
    char rotary_axis_ = 'Y';
    double rotary_scale_ = 0;//to fit gcode unit to circumference
    double circumference_;
    QButtonGroup *axis_group_;
    QButtonGroup *rotation_group_;

signals:
  void rotaryModeChanged(bool is_rotary_mode);
  void mirrorModeChanged(bool is_mirror_mode);
  void rotaryAxisChanged(char axis);
  void actionTestRotary(QRectF bbox, char rotary_axis, qreal feedrate);
  void updateCircumference(double circumference);
};

#endif // ROTARY_SETUP_H
