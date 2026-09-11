#include "refraction-compensator.h"

#include <cmath>

void RefractionCompensator::setParams(const RefractionParams &params) {
  params_ = params;
  if (!std::isfinite(params_.refractive_index) || params_.refractive_index <= 0.0) {
    params_.refractive_index = 1.0;
  }
  if (!std::isfinite(params_.material_height_mm)) {
    params_.material_height_mm = 0.0;
  }
}

double RefractionCompensator::machineZ(double model_z_mm) const {
  const double depth_mm = params_.material_height_mm - model_z_mm;
  return params_.material_height_mm - depth_mm / params_.refractive_index;
}

QString RefractionCompensator::describe() const {
  return QStringLiteral("basic refraction n=%1, material height=%2mm")
      .arg(params_.refractive_index)
      .arg(params_.material_height_mm);
}
