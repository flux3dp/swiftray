#ifndef GCODE_COMMAND_H
#define GCODE_COMMAND_H

#include "operation_cmd.h"
#include <periph/motion_controller/motion_controller.h>

class GCodeCmd : public OperationCmd 
{
public:

  explicit GCodeCmd(QString gcode);

  ExecStatus execute(QPointer<Executor> executor, QPointer<MotionController> motion_controller_) override;

private:
  QString gcode_; // single line of gcode or ctrl cmd for grbl
};

#endif // GCODE_COMMAND_H
