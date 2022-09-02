#pragma once

#include <QString>

class FilePathSettings {
public:
    FilePathSettings() = delete;
    static QString getDefaultFilePath();
    static void setDefaultFilePath(QString path);
    static bool isPathExist(QString path);

};
