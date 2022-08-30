#include "globals.h"

#include <connection/serial-port.h>
#include <machine/machine.h>

SerialPort serial_port;

// TODO: Implement a machine manager and wrap this inside
Machine active_machine;
