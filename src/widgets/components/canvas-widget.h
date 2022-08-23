#pragma once

#include <QQuickWidget>

class CanvasWidget: public QQuickWidget {
Q_OBJECT

public:
    CanvasWidget(QWidget *parent = nullptr);
protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
Q_SIGNALS:
    void dropFile(QPoint point, QString filename);
};
