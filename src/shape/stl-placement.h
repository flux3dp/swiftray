#pragma once

#include <QMatrix4x4>
#include <QString>

/**
    \file stl-placement.h
    \brief Settings carried by the placeholder `<rect data-stl="1">` of an STL object.

    An STL object lives in svgcontent as a placeholder rect: the rect only carries the id, the 3D
    transform and the engraving settings, while the mesh itself arrives through a separate
    `stlObjects` field of the loadSVG payload. Keeping the mesh out of the shape hierarchy is
    deliberate -- src/shape/ only needs "id + transform + engraving settings".
*/
struct StlPlacement {
  enum class Mode {
    Line,
    Dot,
  };

  /**
   * Key into the stlObjects map, taken from the `id` attribute of the placeholder rect.
   * Empty means "this shape is an ordinary path".
   */
  QString id;
  /**
   * 3D transform, already expressed in canvas coordinates by the frontend (unit 0.1mm, canvas Y).
   * It is handed to stl::Slicer::prepare() as is -- the backend does no coordinate conversion.
   */
  QMatrix4x4 matrix;
  /** <= 0 means "use the layer setting" */
  double layer_height_mm = 0.0;
  /** <= 0 means "use the layer setting" */
  double point_spacing_mm = 0.0;
  Mode mode = Mode::Line;

  bool isValid() const { return !id.isEmpty(); }
};
