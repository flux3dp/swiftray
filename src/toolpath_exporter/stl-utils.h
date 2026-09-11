#pragma once

#include <QByteArray>
#include <QMatrix4x4>
#include <QPolygonF>
#include <QString>
#include <QVector>
#include <cstdint>
#include <functional>
#include <vector>

/**
    \file stl-utils.h
    \brief STL loading, transformed-mesh sampling and slicing for UV inner engraving.

    Coordinate / unit contract:
      - X right, Y down, Z up. Slicing planes are horizontal, plane normal = Z.
      - STL files are in mm by convention. The frontend sends a full 4x4 matrix that already folds
        in the mm -> 0.1mm (x10) conversion, so this module never guesses units: it slices in
        whatever unit the transformed mesh ends up in.
      - Layers are ordered deep -> shallow, i.e. bottom -> top / ascending Z, because
        already engraved crack points scatter the laser for the layers behind them.
*/

namespace stl {

/** Return false to cancel; value is the completed fraction in [0, 1]. */
using ProgressCallback = std::function<bool(double)>;

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
 * Fixed seed used by every STL blue-noise sampler.
 *
 * The implementation uses its own integer PRNG and integer-to-double conversion instead of a
 * standard-library distribution, whose exact output is not specified across MSVC/libc++ builds.
 */
constexpr std::uint64_t kBlueNoiseSeed = UINT64_C(0x5357494654524159);  // "SWIFTRAY"

struct SurfacePoint {
  Vec3 position;
  /** 0 is the original surface; positive values are successive inward shells. */
  int shell_index = 0;
};

/** Raw BSPC positions in the frontend object's local millimetre space. */
struct PointCloud {
  std::vector<Vec3> points;

  bool isEmpty() const { return points.empty(); }
};

struct BlueNoiseParams {
  /** Minimum 3D distance between samples on one shell, in transformed mesh units. */
  double spacing = 0.1;
  /** Distance between inward shells. <= 0 uses spacing. */
  double shell_spacing = 0.0;
  /** Safety bound for malformed/huge meshes; high enough for normal exporter work areas. */
  int max_shell_count = 3;
};

struct PointCloudResult {
  bool ok = false;
  QString error;
  std::vector<SurfacePoint> points;
  int shell_count = 0;
  qint64 candidate_count = 0;
  bool shell_limit_reached = false;
};

/** Generate a deterministic Poisson-disk (blue-noise) point cloud on the transformed surface. */
PointCloudResult sampleSurfaceBlueNoise(const Mesh &mesh, const QMatrix4x4 &transform,
                                        const BlueNoiseParams &params,
                                        ProgressCallback progress = {});

/**
 * Generate deterministic blue-noise point clouds on successively inward-offset mesh shells.
 *
 * The transform is applied before normals, distances, bounds, offsets, areas or samples are
 * calculated. Shell zero is the transformed surface. Each later shell uses a separate deterministic
 * random stream derived from kBlueNoiseSeed.
 */
PointCloudResult sampleInwardShellsBlueNoise(const Mesh &mesh, const QMatrix4x4 &transform,
                                             const BlueNoiseParams &params,
                                             ProgressCallback progress = {});

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

/** Parse and validate a Beam Studio Point Cloud (`BSPC`, version 1) binary. */
bool readPointCloud(const QByteArray &data, PointCloud *cloud, QString *error);

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
   * Z of the slicing plane in the transformed model's real geometry space.
   */
  double z_geometry = 0.0;
  /** Empty layers are legal and are kept, so layer index always maps to a Z. */
  QVector<Contour> contours;
};

/**
 * Incremental slicer: prepare a mesh once, then ask for the contours at any Z.
 *
 * This is what lets several objects share one Z ladder: a job with more than one STL engraves
 * strictly bottom to top across ALL objects, so each object has to be sliced at the Z values of the
 * merged ladder rather than at its own private layer positions.
 *
 * @note when @p transform is the identity, prepare() keeps a pointer to @p mesh instead of copying
 *       it (a 700k triangle model is ~50MB), so the mesh must outlive the slicer.
 */
class Slicer {
 public:
  /**
   * @return false and fills @p error when the mesh is empty or the layer height is invalid
   */
  bool prepare(const Mesh &mesh, const QMatrix4x4 &transform, const SliceParams &params,
               QString *error = nullptr, ProgressCallback progress = {});

  bool isReady() const { return ready_; }

  /** Z range of the transformed mesh */
  double minZ() const { return min_z_; }
  double maxZ() const { return max_z_; }

  /** The Z planes this mesh would be sliced at on its own, with the prepared layer height */
  QVector<double> planes() const;

  /**
   * Adaptive Z planes for line engraving.
   *
   * `SliceParams::layer_height` is the maximum step. Steep XY contour motion caused by shallow
   * facets reduces the step down to @p minimum_layer_height. The returned planes still cover the
   * complete transformed Z range and stay strictly bottom-to-top.
   */
  QVector<double> adaptivePlanes(double minimum_layer_height,
                                 ProgressCallback progress = {}) const;

  /**
   * Contours at an arbitrary Z.
   * Non decreasing Z is the fast path (the internal sweep only moves forward); going backwards is
   * still correct, it just restarts the sweep.
   * `index` of the returned layer is left at its default: the caller numbers the layers, because a
   * shared ladder does not map one to one onto a single object.
   */
  Layer sliceAt(double z, ProgressCallback progress = {});

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
  /** XY contour displacement per unit Z for each transformed triangle. */
  std::vector<double> tri_xy_rate_;
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
};

/**
 * Exact greedy nearest-neighbour order for points.
 *
 * The first point is nearest @p anchor; every following point is nearest the previously selected
 * point. A balanced 2D k-d tree avoids the O(n^2) scan used by the small polygon-path helper above.
 * Equal-distance ties use the original point index, keeping output deterministic.
 */
QVector<int> nearestPointOrder(const QPolygonF &points, const QPointF &anchor,
                               ProgressCallback progress = {});

/** Signed area of a polygon, positive means counter clockwise. Handles a repeated last point. */
double signedArea(const QPolygonF &polygon);

}  // namespace stl
