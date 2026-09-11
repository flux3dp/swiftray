#pragma once

#include <QMatrix4x4>
#include <QString>

/**
    \file stl-placement.h
    \brief Settings carried by the SVG projection of a 3D canvas object.

    A mesh or point cloud lives in svgcontent as a placeholder rect while its binary arrives in a
    separate loadSVG field. A photo remains an SVG image and carries these settings alongside its
    pixels. Keeping placement independent of the concrete geometry lets all three enter the same
    Z-ordered output path.
*/
struct StlPlacement {
  enum class GeometryKind {
    Mesh,
    Photo,
    PointCloud,
  };

  enum class Mode {
    Line,
    Dot,
  };

  /**
   * SVG element id. It is also the key into the appropriate binary map for meshes/point clouds.
   * Empty means "this shape is an ordinary 2D shape".
   */
  QString id;
  /**
   * 3D transform, already expressed in canvas coordinates by the frontend (unit 0.1mm, canvas Y).
   * The exporter applies it as-is to the selected geometry -- the backend does no coordinate
   * conversion.
   */
  QMatrix4x4 matrix;
  /** kLine maximum Z step / kDotFill inward-shell spacing; <= 0 uses the exporter default. */
  double layer_height_mm = 0.0;
  /** kLine minimum Z step; > 0 opts the object into adaptive-plane slicing. */
  double min_layer_height_mm = 0.0;
  /** Blue-noise minimum distance on each dot shell; <= 0 uses the exporter default. */
  double point_spacing_mm = 0.0;
  /** Local dimensions of a photo plane before the 4x4 transform is applied. */
  double photo_width_mm = 0.0;
  double photo_height_mm = 0.0;
  /** Which loadSVG binary map owns `id`; absent frontend metadata defaults to the legacy mesh. */
  GeometryKind geometry_kind = GeometryKind::Mesh;
  Mode mode = Mode::Line;

  bool isValid() const { return !id.isEmpty(); }
};
