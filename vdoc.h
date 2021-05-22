#ifndef VDOC_H
#define VDOC_H

#include <QFont>
#include <QObject>
#include <QTextCursor>
#include <QUrl>
#include "vcanvas.h"

QT_BEGIN_NAMESPACE
class QTextDocument;
class QQuickTextDocument;
QT_END_NAMESPACE

class VDoc : public QObject {
        Q_OBJECT
        Q_PROPERTY(QString fileName READ fileName NOTIFY fileUrlChanged)
        Q_PROPERTY(QString fileType READ fileType NOTIFY fileUrlChanged)
        Q_PROPERTY(QUrl fileUrl READ fileUrl NOTIFY fileUrlChanged)
        Q_PROPERTY(VCanvas* canvas READ canvas WRITE setCanvas NOTIFY canvasChanged)

    public:
        VDoc(QObject *parent = nullptr);
        QString fileName() const;
        QString fileType() const;
        QUrl fileUrl() const;
        VCanvas* canvas() const;
        void setCanvas(VCanvas *canvas);
    public Q_SLOTS:
        void load(const QUrl &fileUrl);
        void saveAs(const QUrl &fileUrl);
    Q_SIGNALS:
        void fileUrlChanged();
        void canvasChanged();
        void error(const QString &message);
    private:
        QUrl m_fileUrl;
        VCanvas *m_canvas;
};


#endif // VDOC_H
