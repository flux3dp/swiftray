#include <QMainWindow>
#include <QString>

#ifdef Q_OS_MACOS
#ifndef OSXWINDOW_H
#define OSXWINDOW_H

void setOSXWindowTitleColor(QMainWindow *win);

QString familyNameFromPostScriptName(QString name);

#endif // OSXWINDOW_H
#endif
