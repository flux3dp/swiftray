#ifndef TRANSFORM_WIDGET_H
#define TRANSFORM_WIDGET_H

#include <QFrame>

namespace Ui {
class TransformPanel;
}

class TransformPanel : public QFrame
{
    Q_OBJECT

public:
    explicit TransformPanel(QWidget *parent = nullptr);
    ~TransformPanel();

private:
    Ui::TransformPanel *ui;
};

#endif // TRANSFORM_WIDGET_H
