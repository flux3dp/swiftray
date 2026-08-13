#pragma once

#include <QPointF>
#include <QString>
#include <QVector>

#include "stl-utils.h"

/**
    \file refraction-compensator.h
    \brief Where to actually put the head and aim the galvo so that the focus lands on a point INSIDE
           the crystal (TODO.md B-3, TODO-backend.md I-6).

    The beam is focused through the flat top surface of the workpiece, so it refracts on the way in.
    Two things change, and they are of very different sizes:

    1. **Depth** -- a converging cone entering a denser medium converges more slowly, so the focus
       ends up DEEPER than it would in air. To put the focus at a real depth `d` below the surface,
       the nominal (in air) focal plane has to sit only `t < d` below it, `t = d / n` on the optical
       axis. This is a first order effect: at n = 1.52 it is 34% of the depth, tens of mm.
    2. **Position** -- off axis the beam enters the surface at an angle and is bent towards the
       normal, so it drifts sideways less than it would in air. But the depth compensation is done by
       raising the head, which scales the lateral geometry by the same 1/n, and the two nearly
       cancel: what is left is ~0.05mm at the edge of a 70mm field, 100mm deep.

    Geometry, all distances from the galvo's optical centre P, radii from the centre of the field:

    ~~~
                         P  (optical centre)
                         |\
                         | \  theta1
                    F-t  |  \
                         |   \                      <- design focal plane is t below the surface,
        ---- crystal top -----X------------------      so P is (F - t) above the surface
                         |     \  theta2
                       d |      \
                         |       \
        ---- target ----------- * (r, depth d)
    ~~~

        R = F * tan(theta1)                            (what we command)
        r = (F - t) * tan(theta1) + d * tan(theta2)    (where the focus actually lands)
        sin(theta1) = n * sin(theta2)
        t = d * cos^2(theta1) / (n * cos^2(theta2))    sagittal focus of a refracted cone, = d/n on axis

    ⚠️ `F - t`, not `F`: the Z axis moves the head, so the distance from P to the surface changes
    with the compensation itself. `t` reaches `H / n`, the same order of magnitude as `F`, and
    ignoring it puts the lateral correction off by centimetres instead of tens of microns.

    **The depth part is applied by warping the mesh before slicing** (stl::MeshWarp): `warpZ()`
    returns, for a point of the model, the machine Z that engraves it. A flat plane in that warped
    space is the shallow bowl the machine can really reach at one Z, so the ladder keeps "one layer =
    one Z move" instead of needing a Z move per engraved point. ⚠️ The warp is nonlinear in XY, so
    long triangle edges have to be subdivided first -- maxEdgeLength() sizes that.

    **The lateral part is applied per emitted point** (prepareFocusZ() + mapXY()), because it is a
    distortion of the XY pattern, not of the geometry: one forward sweep over theta1 gives the whole
    `r -> scale` table of a layer, and every point is a binary search in it.

    ⚠️ Accuracy. The model is first order in three ways that matter more than its own arithmetic: a
    real galvo has no single pivot point, an f-theta lens maps `R = f * theta` rather than
    `F * tan(theta)`, and a cone refracted at an oblique angle has two foci (astigmatism) instead of
    one. **The Z direction and the depth scale MUST be verified on a real machine with a small test
    piece before this is trusted.**
*/
struct RefractionParams {
  /** Master switch (`refraction_compensation` convert param), default on. */
  bool enabled = true;
  /** n of the workpiece. <= 1 means "no refraction", the compensation becomes the identity. */
  double refractive_index = 1.0;
  /** H: height of the crystal top surface above the focus origin (the work platform), mm. */
  double material_height_mm = 0.0;
  /** F: optical centre -> design focal plane, mm. <= 0 means unknown -> depth only compensation. */
  double focal_length_mm = 0.0;
};

class RefractionCompensator : public stl::MeshWarp {
 public:
  void setParams(const RefractionParams &params);
  const RefractionParams &params() const { return params_; }

  /**
   * @param center_mm centre of the galvo field, i.e. the centre of the work area
   * @param radius_mm largest target radius that has to be covered by the lateral table
   * @param units_per_mm mesh / document units per mm (10, the canvas unit is 0.1mm)
   */
  void setField(QPointF center_mm, double radius_mm, double units_per_mm);

  /** Refraction actually changes something (switch on, n > 1, a workpiece height is known). */
  bool isActive() const;
  /** The lateral term is only computable when the focal length is known. */
  bool hasLateralTerm() const { return isActive() && params_.focal_length_mm > 0; }

  /**
   * Layer heights are given as a real depth step in the model, but a warped mesh is sliced in
   * machine Z. On axis one is `n` times the other; at the edge of the field the real step comes out
   * ~1% larger, which is far below any layer height that makes sense.
   * @return what the requested layer height has to be multiplied by before slicing
   */
  double layerHeightScale() const { return isActive() ? 1.0 / params_.refractive_index : 1.0; }

  // --- stl::MeshWarp: the depth part, applied to the mesh before slicing ---

  /** @return the machine Z that engraves the point, in mesh units. Identity when inactive. */
  double warpZ(double x, double y, double z) const override;
  /** Edge length at which the warp's curvature starts to matter, in mesh units. */
  double maxEdgeLength() const override;

  // --- the lateral part, applied to every emitted point ---

  /** Build the `r -> scale` table of the layer engraved at @p focus_z_mm (mm above the origin). */
  void prepareFocusZ(double focus_z_mm);

  /** Map a target point of the prepared layer to what the galvo has to be commanded. */
  QPointF mapXY(QPointF xy_mm) const;

  /** One line for the export log. */
  QString describe() const;

 private:
  /** Nominal focus depth below the surface for a field angle, mm. The heart of the model. */
  double focusDepth(double depth_mm, double tan_theta1) const;

  struct Sample {
    double r_mm = 0.0;  // target radius
    double scale = 1.0; // command radius / target radius
  };

  RefractionParams params_;
  QPointF field_center_mm_;
  double field_radius_mm_ = 0.0;
  double units_per_mm_ = 1.0;
  QVector<Sample> table_;
};
