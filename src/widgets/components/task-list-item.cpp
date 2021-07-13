#include "task-list-item.h"
#include "ui_task-list-item.h"

TaskListItem::TaskListItem(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TaskListItem)
{
    ui->setupUi(this);
}

TaskListItem::~TaskListItem()
{
    delete ui;
}
