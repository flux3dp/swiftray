#include "rt_status_update_executor.h"

#include <QDebug>

RTStatusUpdateExecutor::RTStatusUpdateExecutor(MotionController *motion_controller, 
                                              QObject *parent)
  : Executor{parent}, motion_controller_{motion_controller}
{
  qInfo() << "RTStatusUpdateExecutor created";
}

void RTStatusUpdateExecutor::start() {
  qInfo() << "RTStatusUpdateExecutor start()";
}
