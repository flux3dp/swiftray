#include "vdoc.h"
#include <QFile>
#include <QFileInfo>
#include <QFileSelector>
#include <QMimeDatabase>
#include <QQmlFile>
#include <QQmlFileSelector>
#include <QDebug>

VDoc::VDoc(QObject *parent) : QObject(parent) {
}

QString VDoc::fileName() const {
    const QString filePath = QQmlFile::urlToLocalFileOrQrc(m_fileUrl);
    const QString fileName = QFileInfo(filePath).fileName();

    if (fileName.isEmpty())
        return QStringLiteral("untitled.txt");

    return fileName;
}

QString VDoc::fileType() const {
    return QFileInfo(fileName()).suffix();
}

QUrl VDoc::fileUrl() const {
    return m_fileUrl;
}

void VDoc::load(const QUrl &fileUrl) {
    if (fileUrl == m_fileUrl)
        return;

    QQmlEngine *engine = qmlEngine(this);

    if (!engine) {
        qWarning() << "load() called before VDoc has QQmlEngine";
        return;
    }

    const QUrl path = QQmlFileSelector::get(engine)->selector()->select(fileUrl);
    const QString fileName = QQmlFile::urlToLocalFileOrQrc(path);
    qInfo("Loading file ");
    qInfo() << fileName;

    if (QFile::exists(fileName)) {
        QMimeType mime = QMimeDatabase().mimeTypeForFile(fileName);
        QFile file(fileName);

        if (file.open(QFile::ReadOnly)) {
            QByteArray data = file.readAll();
            qInfo() << "File size:" << data.size();
            canvas()->loadSvg(data);
        }
    }

    m_fileUrl = fileUrl;
    emit fileUrlChanged();
}

void VDoc::saveAs(const QUrl &fileUrl) {
    const QString filePath = fileUrl.toLocalFile();
    const bool isHtml = QFileInfo(filePath).suffix().contains(QLatin1String("htm"));
    QFile file(filePath);
    QString data = "Random";

    if (!file.open(QFile::WriteOnly | QFile::Truncate | (isHtml ? QFile::NotOpen : QFile::Text))) {
        emit error(tr("Cannot save: ") + file.errorString());
        return;
    }

    file.write(data.toUtf8());
    file.close();

    if (fileUrl == m_fileUrl)
        return;

    m_fileUrl = fileUrl;
    emit fileUrlChanged();
}

VCanvas *VDoc::canvas() const {
    return m_canvas;
}

void VDoc::setCanvas(VCanvas *_canvas) {
    m_canvas = _canvas;
    qInfo() << "Setting canvas " << _canvas;
}
