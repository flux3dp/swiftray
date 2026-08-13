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
  /**
   * Z of the slicing plane, in the space the mesh was prepared in. Without a MeshWarp that is the
   * geometry's own Z; with one it is the warped sweep coordinate -- for the refractive index
   * compensation, the machine Z that engraves this layer (see refraction-compensator.h).
   */
  double z_geometry = 0.0;
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

/**
 * Bends the sweep axis before slicing, so that a flat slicing plane in the swept space is a curved
 * surface in the real one.
 *
 * This exists for the refractive index compensation (TODO.md B-3): what the machine can reach with
 * ONE Z position is not a flat plane inside the workpiece but a shallow bowl, so slicing at flat
 * heights and correcting afterwards means every single point of a layer needs its own Z move.
 * Slicing in the warped space instead gives back "one layer = one Z", and costs nothing in the
 * slicer: `w = warpZ(x, y, z)` is applied to the vertices, the sweep is unchanged.
 *
 * The implementation is in refraction-compensator.h; nothing in this file knows about optics.
 *
 * ⚠️ Two things the implementer owes the slicer:
 *   - `warpZ` MUST be strictly increasing in z, otherwise the sweep is not a sweep any more.
 *   - the warp is nonlinear in XY, so the straight edges of a triangle are no longer straight after
 *     it. `maxEdgeLength()` says how long an edge may be before prepare() subdivides it.
 */
class MeshWarp {
 public:
  virtual ~MeshWarp() = default;
  /** @return the sweep coordinate of a point, in mesh units. Must be increasing in @p z. */
  virtual double warpZ(double x, double y, double z) const = 0;
  /** Longest edge kept as is, in mesh units. <= 0 disables the subdivision. */
  virtual double maxEdgeLength() const = 0;
};

/**
 * Incremental slicer: prepare a mesh once, then ask for the contours at any Z.
 *
 * This is what lets several objects share one Z ladder: a job with more than one STL engraves
 * strictly bottom to top across ALL objects, so each object has to be sliced at the Z values of the
 * merged ladder rather than at its own private layer positions.
 *
 * @note when @p transform is the identity and there is no warp, prepare() keeps a pointer to
 *       @p mesh instead of copying it (a 700k triangle model is ~50MB), so the mesh must outlive
 *       the slicer.
 */
class Slicer {
 public:
  /**
   * @param warp optional sweep axis warp, see MeshWarp. Not owned, only used during this call.
   *             ⚠️ With a warp the layer height is a step in the WARPED coordinate.
   * @return false and fills @p error when the mesh is empty or the layer height is invalid
   */
  bool prepare(const Mesh &mesh, const QMatrix4x4 &transform, const SliceParams &params,
               QString *error = nullptr, const MeshWarp *warp = nullptr);

  bool isReady() const { return ready_; }

  /** Z range of the transformed mesh */
  double minZ() const { return min_z_; }
  double maxZ() const { return max_z_; }

  /** The Z planes this mesh would be sliced at on its own, with the prepared layer height */
  QVector<double> planes() const;

  /**
   * Contours at an arbitrary Z.
   * Non decreasing Z is the fast path (the internal sweep only moves forward); going backwards is
   * still correct, it just restarts the sweep.
   * `index` and `elapsed_us` of the returned layer are left at their defaults: the caller numbers
   * the layers, because a shared ladder does not map one to one onto a single object.
   */
  Layer sliceAt(double z);

  /** Counters accumulated over every sliceAt() call so far */
  const SliceStats &stats() const { return stats_; }

 private:
  /**
   * The mesh being sliced. A transformed copy is owned, an untransformed one is only referenced --
   * and the flag rather than a pointer decides which, so that moving a Slicer (jobs are collected
   * in a vector) can never leave a pointer aimed at the moved-from object's own member.
   */
  const Mesh &work() const { return owns_mesh_ ? transformed_ : *source_; }

  bool ready_ = false;
  bool owns_mesh_ = false;
  const Mesh *source_ = nullptr;
  Mesh transformed_;
  SliceParams params_;
  std::vector<double> tri_min_z_;
  std::vector<double> tri_max_z_;
  /** Triangle indices sorted by their minimum Z, degenerate ones already dropped */
  std::vector<int> order_;
  /** Triangles straddling the last sliced plane */
  std::vector<int> active_;
  size_t next_triangle_ = 0;
  double last_z_ = 0.0;
  bool swept_ = false;
  double min_z_ = 0.0;
  double max_z_ = 0.0;
  double plane_eps_ = 0.0;
  double weld_tolerance_ = 0.0;
  SliceStats stats_;
};

/** Slice a whole mesh at its own layer positions. Thin wrapper over Slicer. */
SliceResult sliceMesh(const Mesh &mesh, const QMatrix4x4 &transform, const SliceParams &params);

/**
 * Slice several independent objects: each instance keeps its own transform and its
 * own layer list, because multi object jobs engrave one object at a time.
 */
QVector<SliceResult> sliceMeshes(const QVector<MeshInstance> &instances, const SliceParams &params);

/**
 * Resample a contour into EVENLY spaced points for dot mode.
 *
 * The point count is `round(length / spacing)`, and the actual step is then `length / count`, so
 * every gap is identical and only slightly off the requested spacing. Nothing is discarded and no
 * gap is ever shorter than the others -- in particular a closed contour has no over exposed seam.
 *
 *   - open contour:   count + 1 points, both endpoints included
 *   - closed contour: count points, the start is not repeated at the end
 *   - count is clamped to at least 1, so a contour shorter than half a spacing still yields its
 *     endpoints (open) or its start point (closed)
 *
 * @param is_closed whether the contour wraps around. A trailing point equal to the first one is
 *                  dropped either way, so passing a contour in the `Contour::polygon` convention
 *                  works without any preparation.
 */
QPolygonF resamplePolygon(const QPolygonF &polygon, double spacing, bool is_closed);

/** Signed area of a polygon, positive means counter clockwise. Handles a repeated last point. */
double signedArea(const QPolygonF &polygon);

}  // namespace stl
