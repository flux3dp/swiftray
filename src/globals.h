#ifndef SWIFTRAY_GLOBALS_H
#define SWIFTRAY_GLOBALS_H

#ifdef CUSTOM_SERIAL_PORT_LIB
#include <connection/serial-port.h>
#else
#include <QSerialPort>
#endif

#include <machine/machine.h>
#include <QList>
#include <QPointer>

#ifdef CUSTOM_SERIAL_PORT_LIB
extern SerialPort serial_port;
#else
extern QSerialPort serial_port;
#endif

extern QList<Machine*> machine_list;
//extern QPointer<Machine> active_machine;
extern Machine active_machine;

#endif //SWIFTRAY_GLOBALS_H
