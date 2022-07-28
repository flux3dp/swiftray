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
            emit dropFile(event->pos(), url.path());
        }
    }
}
