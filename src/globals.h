#ifndef SWIFTRAY_GLOBALS_H
#define SWIFTRAY_GLOBALS_H

#include <connection/serial-port.h>
#include <machine/machine.h>
#include <QList>
#include <QPointer>

extern SerialPort serial_port;

extern QList<Machine*> machine_list;
//extern QPointer<Machine> active_machine;
extern Machine active_machine;

#endif //SWIFTRAY_GLOBALS_H
