#ifndef GCODE_COMMAND_H
#define GCODE_COMMAND_H

#include "operation_cmd.h"
#include <periph/motion_controller/motion_controller.h>

class GCodeCmd : public OperationCmd 
{
public:

  explicit GCodeCmd();
  void setMotionController(QPointer<MotionController> motion_controller);
  void setGCode(QString gcode);

  ExecStatus execute(QPointer<Executor> executor) override;

private:
  QPointer<MotionController> motion_controller_;
  QString gcode_; // single line of gcode or ctrl cmd for grbl
};

#endif // GCODE_COMMAND_H
