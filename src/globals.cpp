#include "globals.h"

#ifdef CUSTOM_SERIAL_PORT_LIB
#include <connection/serial-port.h>
#else
#include <QSerialPort>
#endif
#include <machine/machine.h>

#ifdef CUSTOM_SERIAL_PORT_LIB
SerialPort serial_port;
#else
QSerialPort serial_port;
#endif

// TODO: Implement a machine manager and wrap this inside
QList<Machine*> machine_list;
//QPointer<Machine> active_machine;
Machine active_machine;
