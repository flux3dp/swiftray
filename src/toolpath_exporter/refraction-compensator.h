#pragma once

#include <QString>

struct RefractionParams {
  double refractive_index = 1.0;
  double material_height_mm = 0.0;
};

/**
 * Applies the basic axial refraction model to an entire model-Z plane:
 *
 *   machine_z = material_height - (material_height - model_z) / refractive_index
 *
 * The conversion is always applied. A refractive index of 1 keeps the original Z value.
 */
class RefractionCompensator {
 public:
  void setParams(const RefractionParams &params);
  const RefractionParams &params() const { return params_; }

  double machineZ(double model_z_mm) const;
  QString describe() const;

 private:
  RefractionParams params_;
};
