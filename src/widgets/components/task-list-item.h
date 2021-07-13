#ifndef TASKLISTITEM_H
#define TASKLISTITEM_H

#include <QWidget>

namespace Ui {
class TaskListItem;
}

class TaskListItem : public QWidget
{
    Q_OBJECT

public:
    explicit TaskListItem(QWidget *parent = nullptr);
    ~TaskListItem();

private:
    Ui::TaskListItem *ui;
};

#endif // TASKLISTITEM_H
