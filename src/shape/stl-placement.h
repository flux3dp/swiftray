#pragma once

#include <QMatrix4x4>
#include <QString>

// esther review: Attribute 說明要保留嗎？
/**
    \file stl-placement.h
    \brief Settings carried by the placeholder `<rect data-stl="...">` of an STL object.

    An STL object lives in svgcontent as a placeholder rect: the rect only carries the id, the 3D
    transform and the engraving settings, while the mesh itself arrives through a separate
    `stlObjects` field of the loadSVG payload. Keeping the mesh out of the shape hierarchy is
    deliberate -- src/shape/ only needs "id + transform + engraving settings".

    Attribute contract with the frontend (all optional except `data-stl`):

    | attribute                | meaning                                                        |
    | ------------------------ | -------------------------------------------------------------- |
    | `data-stl`               | object id, the key into the `stlObjects` map                    |
    | `data-stl-matrix`        | 16 numbers, COLUMN major (same order as `THREE.Matrix4.elements`
    |                          | and CSS `matrix3d()`), already in canvas coordinates: the mm -> |
    |                          | 0.1mm x10 and the Y axis conversion are both done by the        |
    |                          | frontend                                                        |
    | `data-stl-layer-height`  | layer height in mm; <= 0 falls back to the layer setting        |
    | `data-stl-point-spacing` | dot mode point spacing in mm; <= 0 falls back to the layer      |
    | `data-stl-mode`          | "dot" or "line" (default line)                                  |
    | `data-stl-fill`          | "1" filled, "0" outline only; absent follows the layer type     |

    ⚠️ The placeholder rect must never be engraved as a rectangle.
*/
struct StlPlacement {
  enum class Mode {
    Line,
    Dot,
  };

  /** Key into the stlObjects map. Empty means "this shape is an ordinary path". */
  QString id;
  // esther review: TBC
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
  // esther review: this might not need ot be here. it can follow the path shape settings
  /** -1 unset (follow the layer type), 0 outline only, 1 filled */
  int fill = -1;

  bool isValid() const { return !id.isEmpty(); }
};
