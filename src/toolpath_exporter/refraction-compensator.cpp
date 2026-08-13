#include "refraction-compensator.h"

#include <QDebug>
#include <algorithm>
#include <cmath>

namespace {
/** Samples of the theta1 sweep. The curve is smooth and monotonic, this is plenty. */
constexpr int kTableSamples = 256;
constexpr double kEps = 1e-9;
/**
 * How far a warped triangle edge may deviate from the surface it should follow, mm. The warp is a
 * shallow paraboloid in r, so the deviation over an edge of length L is about `k * L^2 / 8`; 10um
 * is an order of magnitude below the spot and below any sane layer height.
 */
constexpr double kWarpToleranceMm = 0.01;
}  // namespace

void RefractionCompensator::setParams(const RefractionParams &params) {
  params_ = params;
  table_.clear();
}

void RefractionCompensator::setField(QPointF center_mm, double radius_mm, double units_per_mm) {
  field_center_mm_ = center_mm;
  field_radius_mm_ = radius_mm;
  units_per_mm_ = units_per_mm > 0 ? units_per_mm : 1.0;
  table_.clear();
}

bool RefractionCompensator::isActive() const {
  return params_.enabled && params_.refractive_index > 1.0 && params_.material_height_mm > 0.0;
}

/**
 * Nominal focus depth for a target at @p depth_mm below the surface, seen at field angle theta1:
 * `t = d * cos^2(theta1) / (n * cos^2(theta2))`, which is `d / n` on the axis and grows slightly
 * shallower off axis (a more oblique cone is squeezed harder by the refraction).
 */
double RefractionCompensator::focusDepth(double depth_mm, double tan_theta1) const {
  const double n = params_.refractive_index;
  const double cos1 = 1.0 / std::sqrt(1.0 + tan_theta1 * tan_theta1);
  const double sin1 = tan_theta1 * cos1;
  const double sin2 = sin1 / n;
  const double cos2_sq = std::max(kEps, 1.0 - sin2 * sin2);
  return depth_mm * (cos1 * cos1) / (n * cos2_sq);
}

double RefractionCompensator::warpZ(double x, double y, double z) const {
  if (!isActive()) return z;

  const double z_mm = z / units_per_mm_;
  // NOT clamped to the workpiece: a point above the top surface gets a negative depth, which keeps
  // warpZ strictly increasing in z. A model sticking out of the material is the frontend's problem,
  // clamping here would only break the sweep.
  const double depth_mm = params_.material_height_mm - z_mm;
  double tan_theta1 = 0.0;
  if (hasLateralTerm()) {
    const double dx = x / units_per_mm_ - field_center_mm_.x();
    const double dy = y / units_per_mm_ - field_center_mm_.y();
    // The command radius and the target radius agree to ~0.1%, so the target radius is a good
    // enough field angle -- it only feeds the sub percent cos^2 term.
    tan_theta1 = std::hypot(dx, dy) / params_.focal_length_mm;
  }
  const double focus_z_mm = params_.material_height_mm - focusDepth(depth_mm, tan_theta1);
  return focus_z_mm * units_per_mm_;
}

double RefractionCompensator::maxEdgeLength() const {
  // Without the field angle term the warp is a pure scaling of z: linear, so edges stay straight
  // and nothing has to be subdivided.
  if (!isActive() || !hasLateralTerm()) return 0.0;

  const double n = params_.refractive_index;
  const double f = params_.focal_length_mm;
  // w(r) ~= const + k * r^2 with k = (d/n)(1 - 1/n^2)/F^2, worst case at the deepest point.
  const double k = (params_.material_height_mm / n) * (1.0 - 1.0 / (n * n)) / (f * f);
  if (k <= kEps) return 0.0;
  const double length_mm = std::sqrt(8.0 * kWarpToleranceMm / k);
  return std::clamp(length_mm, 1.0, 100.0) * units_per_mm_;
}

void RefractionCompensator::prepareFocusZ(double focus_z_mm) {
  table_.clear();
  if (!hasLateralTerm() || field_radius_mm_ <= 0.0) return;

  const double n = params_.refractive_index;
  const double f = params_.focal_length_mm;
  // The layer is the set of points the machine reaches at this Z, so its nominal focus depth is
  // fixed and the target depth is what varies over the field.
  const double t = params_.material_height_mm - focus_z_mm;
  if (t >= f) {
    qWarning() << "[Refraction] focus depth" << t << "mm exceeds the focal length" << f
               << "mm, the lateral compensation is dropped";
    return;
  }

  // One forward sweep over the field angle: every sample is a complete answer for one angle, so no
  // root finding is needed. r is monotonic in theta1 by construction, hence the binary search in
  // mapXY(). A 20% margin covers the field (r and R agree to ~0.1%), and the loop grows it if not.
  double tan_max = field_radius_mm_ * 1.2 / f;
  for (int attempt = 0; attempt < 8; ++attempt) {
    const double theta_max = std::atan(tan_max);
    table_.clear();
    table_.reserve(kTableSamples + 1);
    for (int i = 0; i <= kTableSamples; ++i) {
      const double theta1 = theta_max * i / kTableSamples;
      const double sin1 = std::sin(theta1);
      const double cos1 = std::cos(theta1);
      const double tan1 = std::tan(theta1);
      const double sin2 = sin1 / n;
      const double cos2 = std::sqrt(std::max(kEps, 1.0 - sin2 * sin2));
      const double tan2 = sin2 / cos2;
      // Inverse of focusDepth(): at this Z, how deep does the beam of this angle reach?
      const double depth_mm = n * t * (cos2 * cos2) / (cos1 * cos1);

      Sample sample;
      // What we command vs where the focus lands. The head is up by (H - t), so the beam crosses
      // the top surface (f - t) below the optical centre, not f.
      const double command_r = f * tan1;
      sample.r_mm = (f - t) * tan1 + depth_mm * tan2;
      sample.scale = sample.r_mm > kEps ? command_r / sample.r_mm : 1.0;
      if (i > 0 && sample.r_mm <= table_.last().r_mm) {
        // Monotonicity is what makes the binary search valid. It cannot fail for sane inputs, but a
        // silently unsorted table would be a nightmare to debug.
        qWarning() << "[Refraction] the r(theta) table is not monotonic, dropping the lateral term";
        table_.clear();
        return;
      }
      table_.append(sample);
    }
    if (!table_.isEmpty() && table_.last().r_mm >= field_radius_mm_) return;
    tan_max *= 1.5;
  }
  qWarning() << "[Refraction] could not cover a field radius of" << field_radius_mm_
             << "mm, dropping the lateral term";
  table_.clear();
}

QPointF RefractionCompensator::mapXY(QPointF xy_mm) const {
  if (table_.isEmpty()) return xy_mm;  // inactive, depth only, or prepareFocusZ() not called

  const QPointF offset = xy_mm - field_center_mm_;
  const double r = std::hypot(offset.x(), offset.y());
  if (r <= kEps) return xy_mm;

  // Binary search + linear interpolation. Points outside the table (outside the work area) keep the
  // outermost scale: they are already out of the machine's reach, the boundary check reports them.
  const auto it = std::lower_bound(table_.cbegin(), table_.cend(), r,
                                   [](const Sample &s, double value) { return s.r_mm < value; });
  double scale = 1.0;
  if (it == table_.cbegin()) {
    scale = it->scale;
  } else if (it == table_.cend()) {
    scale = table_.last().scale;
  } else {
    const Sample &high = *it;
    const Sample &low = *(it - 1);
    const double span = high.r_mm - low.r_mm;
    const double ratio = span > kEps ? (r - low.r_mm) / span : 0.0;
    scale = low.scale + (high.scale - low.scale) * ratio;
  }
  return field_center_mm_ + offset * scale;
}

QString RefractionCompensator::describe() const {
  if (!params_.enabled) return QStringLiteral("refraction compensation off");
  if (!isActive()) {
    return QStringLiteral("refraction compensation inactive (n=%1, material height=%2mm)")
        .arg(params_.refractive_index)
        .arg(params_.material_height_mm);
  }
  return QStringLiteral(
             "refraction compensation n=%1, material height=%2mm, focal length=%3mm (%4), "
             "layer height x%5")
      .arg(params_.refractive_index)
      .arg(params_.material_height_mm)
      .arg(params_.focal_length_mm)
      .arg(hasLateralTerm() ? QStringLiteral("depth + lateral") : QStringLiteral("depth only"))
      .arg(layerHeightScale());
}
