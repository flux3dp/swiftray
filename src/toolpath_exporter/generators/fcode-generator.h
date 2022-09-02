#pragma once

#include <sstream>
#include <toolpath_exporter/generators/base-generator.h>

#ifndef FCODE_GENERATOR_H
#define FCODE_GENERATOR_H

/*
Fcode (Binary gcode) generator for FLUX Beambox and beamo
*/
class FCodeGenerator : public BaseGenerator {
public:
  FCodeGenerator() : BaseGenerator() {

  }
};
