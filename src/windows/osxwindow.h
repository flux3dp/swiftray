#pragma once

#ifdef Q_OS_MACOS

#include <QMainWindow>
#include <QString>

void setOSXWindowTitleColor(QMainWindow *win);
bool isDarkMode();

QString familyNameFromPostScriptName(QString name);

#endif
