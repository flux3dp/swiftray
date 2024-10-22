#pragma once
#include <QMetaType>

struct LayerParameters { // This is used in the UI
double strength = 0.0;
double speed = 0.0;
int repeat = 0;
double backlash = 0.0;
int frequency = 0;
int pulse_width = 0;
};

// Register the type for Qt's meta-object system
Q_DECLARE_METATYPE(LayerParameters);