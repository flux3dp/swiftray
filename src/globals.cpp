#include "globals.h"
#include <QSerialPort>
#include <machine/machine.h>

QSerialPort serial_port;
MachineSettings::MachineParam default_param;
Machine active_machine = Machine(default_param);