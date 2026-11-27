#pragma once

#include "protocol.h"
#include <string>

namespace beamgo {
struct CompileOptions {
  double field_mm = 90;
  bool preview = false;
  int framing_power = 15;
  bool mirror_x = false;
};

// Validates the complete G-code before returning any device records. Coordinates are
// top-left millimetres, as emitted by Swiftray's shared GCodeGenerator.
Bytes compile(const std::string& gcode, const CompileOptions& options = {});
}  // namespace beamgo
