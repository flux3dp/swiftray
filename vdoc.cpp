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
    const QString file_path = QQmlFile::urlToLocalFileOrQrc(this->file_url_);
    const QString file_name = QFileInfo(file_path).fileName();

    if (file_name.isEmpty())
        return QStringLiteral("untitled.txt");

    return file_name;
}

QString VDoc::fileType() const {
    return QFileInfo(fileName()).suffix();
}

QUrl VDoc::fileUrl() const {
    return this->file_url_;
}

void VDoc::load(const QUrl &file_url) {
    if (file_url == this->file_url_)
        return;

    qInfo() << "Loading file" << file_url;
    const QString fileName = file_url.toString();
    qInfo() << "File name" << fileName;

    if (QFile::exists(fileName)) {
        QMimeType mime = QMimeDatabase().mimeTypeForFile(fileName);
        QFile file(fileName);

        if (file.open(QFile::ReadOnly)) {
            QByteArray data = file.readAll();
            qInfo() << "File size:" << data.size();
            canvas()->loadSvg(data);
        }
    }

    this->file_url_ = file_url;
    emit fileUrlChanged();
}

void VDoc::saveAs(const QUrl &file_url) {
    const QString filePath = file_url.toLocalFile();
    const bool isHtml = QFileInfo(filePath).suffix().contains(QLatin1String("htm"));
    QFile file(filePath);
    QString data = "Random";

    if (!file.open(QFile::WriteOnly | QFile::Truncate | (isHtml ? QFile::NotOpen : QFile::Text))) {
        emit error(tr("Cannot save: ") + file.errorString());
        return;
    }

    file.write(data.toUtf8());
    file.close();

    if (file_url == this->file_url_)
        return;

    this->file_url_ = file_url;
    emit fileUrlChanged();
}

VCanvas *VDoc::canvas() const {
    return this->canvas_;
}

void VDoc::setCanvas(VCanvas *canvas) {
    this->canvas_ = canvas;
    qInfo() << "Setting canvas " << this->canvas_;
}
