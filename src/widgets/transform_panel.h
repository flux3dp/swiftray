#ifndef TRANSFORM_WIDGET_H
#define TRANSFORM_WIDGET_H

#include <QFrame>
#include <QDebug>
#include <shape/shape.h>
#include <canvas/controls/transform.h>

namespace Ui {
class TransformPanel;
}

class TransformPanel : public QFrame
{
    Q_OBJECT

public:
    explicit TransformPanel(QWidget *parent = nullptr);
    ~TransformPanel();
    bool isScaleLock() const;
    void setScaleLock(bool scaleLock);
    void setTransformControl(Controls::Transform *ctrl);
    void updateControl();

private:
    void loadStyles();
    void registerEvents();

    Ui::TransformPanel *ui;
    double x_;
    double y_;
    double r_;
    double w_;
    double h_;
    bool scale_lock_;
    Controls::Transform *ctrl_;
};

#endif // TRANSFORM_WIDGET_H
