#pragma once

#ifdef Q_OS_MACOS

#include <QMainWindow>
#include <QString>

void setOSXWindowTitleColor(QMainWindow *win);

QString familyNameFromPostScriptName(QString name);

#endif