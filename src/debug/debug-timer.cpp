#include "debug-timer.h"
#include <QDebug>
#include <QElapsedTimer>

QElapsedTimer timer;
int getDebugTime() {
    if (!timer.isValid()) {
        timer.start();
        qInfo() << "Global Timer started";
    }
    return timer.elapsed();
}