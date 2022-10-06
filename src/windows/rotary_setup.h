#ifndef ROTARY_SETUP_H
#define ROTARY_SETUP_H

#include <QDialog>

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

private:
    Ui::RotarySetup *ui;
    void testRotary();

    bool is_rotary_mode_ = false;
    bool is_mirror_mode_ = false;
    char rotary_axis_ = 'Y';
    double mm_per_rotation_;
    double roller_diameter_;
    double object_diameter_;
    double circumference_;

signals:
  void rotaryModeChanged(bool is_rotary_mode);
  void mirrorModeChanged(bool is_mirror_mode);
  void rotaryAxisChanged(char axis);
  void actionTestRotary(QRectF bbox, char rotary_axis, qreal feedrate);
};

#endif // ROTARY_SETUP_H
