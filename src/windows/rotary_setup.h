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

    QString getRotaryAxis();

    void setRotaryMode(bool is_rotary_mode);

    void setMirrorMode(bool is_mirror_mode);

    void setRotaryAxis(QString rotary_axis);

private:
    Ui::RotarySetup *ui;
    void testRotaryAxis();

    bool is_rotary_mode_ = false;
    bool is_mirror_mode_ = false;
    QString rotary_axis_ = "Y";
    double mm_per_rotation_;
    double roller_diameter_;
    double object_diameter_;
    double circumference_;

signals:
  void rotaryModeChanged(bool is_rotary_mode);
  void mirrorModeChanged(bool is_mirror_mode);
  void rotaryAxisChanged(QString axis);
};

#endif // ROTARY_SETUP_H
