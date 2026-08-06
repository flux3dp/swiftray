#pragma once

#include <QByteArray>
#include <QMatrix4x4>
#include <QPolygonF>
#include <QString>
#include <QVector>
#include <vector>

/**
    \file stl-utils.h
    \brief STL mesh loading and horizontal slicing for UV inner engraving (FLUX crystal inner carving).

    Coordinate / unit contract:
      - X right, Y down, Z up. Slicing planes are horizontal, plane normal = Z.
      - STL files are in mm by convention. The frontend sends a full 4x4 matrix that already folds
        in the mm -> 0.1mm (x10) conversion, so this module never guesses units: it slices in
        whatever unit the transformed mesh ends up in.
      - Layers are ordered deep -> shallow, i.e. bottom -> top / ascending Z, because
        already engraved crack points scatter the laser for the layers behind them.
*/

namespace stl {

struct Vec3 {
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
};

struct Triangle {
  Vec3 v[3];
};

enum class Format {
  Unknown,
  Binary,
  Ascii,
};

struct Mesh {
  std::vector<Triangle> triangles;

  bool isEmpty() const { return triangles.empty(); }

  /** @return false when the mesh is empty, otherwise fills the axis aligned bounding box */
  bool boundingBox(Vec3 *min_corner, Vec3 *max_corner) const;
};

/**
 * Detect the STL flavour WITHOUT trusting the "solid" prefix.
 *
 * binary STL files written by many CAD tools start with the ASCII text "solid ...",
 * so `startswith("solid")` misdetects them as ASCII. The only reliable test is the size identity
 * `84 + faceCount * 50 == fileSize` with faceCount read as a little endian uint32 at offset 80.
 */
Format detectFormat(const QByteArray &data);

/** Parse an STL blob (binary or ASCII). @return false and fills @p error on malformed input. */
bool readMesh(const QByteArray &data, Mesh *mesh, QString *error);

/** Read and parse an STL file. @return false and fills @p error when unreadable or malformed. */
bool readMeshFromFile(const QString &file_path, Mesh *mesh, QString *error);

/**
 * Apply a 4x4 transform in place.
 * A negative determinant (mirroring) is allowed -- contour winding is normalised after slicing,
 * so a mirrored mesh still produces correctly oriented outer contours and holes.
 */
void applyTransform(Mesh *mesh, const QMatrix4x4 &transform);

struct SliceParams {
  /** Distance between slicing planes, in mesh units. Must be > 0. */
  double layer_height = 0.1;
  /**
   * Endpoint welding tolerance used while chaining intersection segments.
   * 0 means auto: bounding box diagonal * 1e-7, which stays far below any real feature while
   * absorbing the float noise of STL coordinates.
   */
  double weld_tolerance = 0.0;
};

struct Contour {
  /** Closed contours repeat the first point as the last point, open polylines do not. */
  QPolygonF polygon;
  bool is_closed = false;
  /** +1 outer contour (CCW), -1 hole (CW), 0 open polyline / undetermined */
  int orientation = 0;
  /** Index of the directly enclosing contour within the same layer, -1 when outermost */
  int parent_index = -1;
  /** Nesting depth: 0 outer, 1 hole, 2 island inside a hole, ... */
  int depth = 0;
};

struct Layer {
  int index = 0;
  /** Pure geometric Z of the slicing plane */
  double z_geometry = 0.0;
  /**
   * Z after refractive index compensation (TODO.md B-3).
   * Currently equal to z_geometry -- the compensation formula is still to be confirmed, the field
   * exists so adding it later does not change this struct.
   */
  double z_compensated = 0.0;
  /** Empty layers are legal and are kept, so layer index always maps to a Z. */
  QVector<Contour> contours;
  /** Time spent on this layer, to spot the layers that dominate the run, this is for dev estimates */
  qint64 elapsed_us = 0;
};

struct SliceStats {
  int triangle_count = 0;
  int layer_count = 0;
  int contour_count = 0;
  int open_contour_count = 0;
  int empty_layer_count = 0;
  /** Triangles lying in a slicing plane, skipped on purpose */
  int coplanar_triangle_count = 0;
  /** Zero area triangles, skipped */
  int degenerate_triangle_count = 0;
  /** Chaining junctions with more than two incident segments (non-manifold) */
  int non_manifold_junction_count = 0;
  qint64 elapsed_ms = 0;
};

struct SliceResult {
  bool ok = false;
  QString error;
  QVector<Layer> layers;
  /** Statistics about the slicing process, for dev estimates */
  SliceStats stats;
};

struct MeshInstance {
  Mesh mesh;
  QMatrix4x4 transform;
};

/** Slice a single mesh. The transform is applied to a private copy, @p mesh is left untouched. */
SliceResult sliceMesh(const Mesh &mesh, const QMatrix4x4 &transform, const SliceParams &params);

/**
 * Slice several independent objects: each instance keeps its own transform and its
 * own layer list, because multi object jobs engrave one object at a time.
 */
QVector<SliceResult> sliceMeshes(const QVector<MeshInstance> &instances, const SliceParams &params);

/**
 * Resample a contour into evenly spaced points for dot mode:
 *   - sample positions are k * spacing for k = 0 .. floor(length / spacing)
 *   - a closed contour never repeats its start point
 *   - a trailing piece shorter than one spacing is DISCARDED (no extra point, no redistribution)
 *   - a contour shorter than one spacing still yields its start point
 * Known side effect: on closed contours the seam gap is `length mod spacing`, which can be much
 * smaller than the spacing and therefore slightly over-exposed.
 */
QPolygonF resamplePolygon(const QPolygonF &polygon, double spacing, bool is_closed);

/** Signed area of a polygon, positive means counter clockwise. Handles a repeated last point. */
double signedArea(const QPolygonF &polygon);

}  // namespace stl
