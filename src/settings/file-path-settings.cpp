#include <settings/file-path-settings.h>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>

QString FilePathSettings::getDefaultFilePath() {
  QString default_file_dir;
  QSettings settings;
  if ( ! settings.contains("defaultFileDir")) { // fall back to desktop path
    QStringList desktop_dir = QStandardPaths::standardLocations(
            QStandardPaths::StandardLocation::DesktopLocation);
    settings.setValue("defaultFileDir", desktop_dir.at(0));
  }
  default_file_dir = settings.value("defaultFileDir").toString();
  if ( ! isPathExist(default_file_dir)) {
    default_file_dir = ".";
  }
  return default_file_dir;
}

void FilePathSettings::setDefaultFilePath(QString path) {
  QSettings settings;
  if (QDir(path).exists()) {
    settings.setValue("defaultFileDir", path);
  } else { // fall back to desktop path
    QStringList desktop_dir = QStandardPaths::standardLocations(
            QStandardPaths::StandardLocation::DesktopLocation);
    settings.setValue("defaultFileDir", desktop_dir.at(0));
  }
}

bool FilePathSettings::isPathExist(QString path) {
  const QFileInfo outputDir(path);
  if ((!outputDir.exists()) || (!outputDir.isDir()) || (!outputDir.isWritable())) {
    //qWarning() << "output directory does not exist, is not a directory, or is not writeable"
    //           << outputDir.absoluteFilePath();
    return false;
  }
  return true;
}