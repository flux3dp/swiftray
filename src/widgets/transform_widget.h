#ifndef TRANSFORM_WIDGET_H
#define TRANSFORM_WIDGET_H

#include <QFrame>

namespace Ui {
class TransformWidget;
}

class TransformWidget : public QFrame
{
    Q_OBJECT

public:
    explicit TransformWidget(QWidget *parent = nullptr);
    ~TransformWidget();

private:
    Ui::TransformWidget *ui;
};

#endif // TRANSFORM_WIDGET_H
