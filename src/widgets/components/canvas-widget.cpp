#include "canvas-widget.h"
#include <QMimeData>

CanvasWidget::CanvasWidget(QWidget *parent) : QQuickWidget(parent) 
{
    setAcceptDrops(true);
}

void CanvasWidget::dragEnterEvent(QDragEnterEvent *event) 
{
     event->acceptProposedAction();
}

void CanvasWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
}

void CanvasWidget::dragMoveEvent(QDragMoveEvent *event)
{
}

void CanvasWidget::dropEvent(QDropEvent *event)
{
    // qInfo() << event->pos();
    if (event->mimeData()->hasUrls()) {
        foreach (QUrl url, event->mimeData()->urls()) {
            // qInfo() << url.path();
            QString file_path = url.path();
#ifdef Q_OS_WIN
            // remove string start with "/"
            file_path = file_path.right(file_path.size()-1);
#endif
            emit dropFile(event->pos(), file_path);
        }
    }
}
