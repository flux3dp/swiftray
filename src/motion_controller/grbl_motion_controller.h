#ifndef GRBLMOTIONCONTROLLER_H
#define GRBLMOTIONCONTROLLER_H

#include "motion_controller.h"

class GrblMotionController : public MotionController
{
public:
  explicit GrblMotionController(QObject *parent = nullptr);
};

#endif // GRBLMOTIONCONTROLLER_H
