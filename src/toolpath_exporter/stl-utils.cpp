#include "stl-utils.h"

#include <QHash>
#include <QRectF>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <map>
#include <queue>
#include <tuple>
#include <unordered_map>
#include <utility>

namespace stl {

namespace {

constexpr qint64 kBinaryHeaderSize = 84;
constexpr qint64 kBinaryTriangleSize = 50;
constexpr qint64 kPointCloudHeaderSize = 12;
constexpr qint64 kPointCloudPositionSize = 3 * sizeof(float);

quint32 readUint32LE(const char *p) {
  return static_cast<quint32>(static_cast<quint8>(p[0])) |
         (static_cast<quint32>(static_cast<quint8>(p[1])) << 8) |
         (static_cast<quint32>(static_cast<quint8>(p[2])) << 16) |
         (static_cast<quint32>(static_cast<quint8>(p[3])) << 24);
}

float readFloatLE(const char *p) {
  const quint32 bits = readUint32LE(p);
  float value = 0.0f;
  std::memcpy(&value, &bits, sizeof(value));
  return value;
}

bool isAsciiSpace(char c) {
  return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v';
}

/**
 * Chaining helper: welds segment endpoints that are within a tolerance of each other.
 * Neighbouring cells are searched as well, otherwise two points a hair apart but on opposite sides
 * of a cell border would never be welded.
 */
class PointWelder {
 public:
  explicit PointWelder(double tolerance)
      : tolerance_(tolerance), inv_cell_(1.0 / std::max(tolerance, 1e-12)) {}

  int insert(double x, double y) {
    const qint64 ix = static_cast<qint64>(std::floor(x * inv_cell_));
    const qint64 iy = static_cast<qint64>(std::floor(y * inv_cell_));
    for (qint64 dx = -1; dx <= 1; ++dx) {
      for (qint64 dy = -1; dy <= 1; ++dy) {
        auto it = cells_.constFind(cellKey(ix + dx, iy + dy));
        if (it == cells_.constEnd()) continue;
        for (int id : *it) {
          const QPointF &q = points_[id];
          if (std::abs(q.x() - x) <= tolerance_ && std::abs(q.y() - y) <= tolerance_) return id;
        }
      }
    }
    const int id = static_cast<int>(points_.size());
    points_.push_back(QPointF(x, y));
    cells_[cellKey(ix, iy)].push_back(id);
    return id;
  }

  const std::vector<QPointF> &points() const { return points_; }

  void clear() {
    points_.clear();
    cells_.clear();
  }

 private:
  static qint64 cellKey(qint64 ix, qint64 iy) {
    // Hash signed indices as unsigned integers; left-shifting a negative signed value is undefined
    // and produced target-dependent behaviour between MSVC and Clang.
    const quint64 x = static_cast<quint64>(ix);
    const quint64 y = static_cast<quint64>(iy);
    return static_cast<qint64>((x * UINT64_C(0x9e3779b97f4a7c15)) ^
                              (y + UINT64_C(0x517cc1b727220a95)));
  }

  double tolerance_;
  double inv_cell_;
  std::vector<QPointF> points_;
  QHash<qint64, std::vector<int>> cells_;
};

struct Chain {
  std::vector<int> point_ids;
  bool closed = false;
};

/**
 * Link unordered intersection segments into ordered polylines.
 * Open chains are grown first (starting from endpoints that are not part of exactly two segments),
 * so a broken mesh yields open polylines instead of arbitrarily cut loops.
 */
std::vector<Chain> chainSegments(const std::vector<std::pair<int, int>> &segments,
                                 int point_count) {
  std::vector<Chain> chains;
  if (segments.empty() || point_count <= 0) return chains;

  std::vector<std::vector<int>> incident(static_cast<size_t>(point_count));
  for (size_t i = 0; i < segments.size(); ++i) {
    incident[static_cast<size_t>(segments[i].first)].push_back(static_cast<int>(i));
    incident[static_cast<size_t>(segments[i].second)].push_back(static_cast<int>(i));
  }

  std::vector<char> used(segments.size(), 0);
  std::vector<int> cursor(static_cast<size_t>(point_count), 0);

  auto takeUnused = [&](int point) -> int {
    std::vector<int> &list = incident[static_cast<size_t>(point)];
    int &pos = cursor[static_cast<size_t>(point)];
    while (pos < static_cast<int>(list.size())) {
      const int seg = list[static_cast<size_t>(pos)];
      if (!used[static_cast<size_t>(seg)]) return seg;
      ++pos;
    }
    return -1;
  };

  auto walk = [&](int start) {
    Chain chain;
    chain.point_ids.push_back(start);
    int current = start;
    while (true) {
      const int seg = takeUnused(current);
      if (seg < 0) break;
      used[static_cast<size_t>(seg)] = 1;
      const std::pair<int, int> &s = segments[static_cast<size_t>(seg)];
      const int next = (s.first == current) ? s.second : s.first;
      chain.point_ids.push_back(next);
      current = next;
      if (current == start) {
        chain.closed = true;
        break;
      }
    }
    return chain;
  };

  // Pass 1: everything that is not a clean two-segment junction is a chain start candidate.
  for (int p = 0; p < point_count; ++p) {
    if (incident[static_cast<size_t>(p)].size() == 2) continue;
    while (takeUnused(p) >= 0) {
      Chain chain = walk(p);
      if (chain.point_ids.size() >= 2) chains.push_back(std::move(chain));
    }
  }
  // Pass 2: whatever is left forms closed loops.
  for (size_t i = 0; i < segments.size(); ++i) {
    if (used[i]) continue;
    Chain chain = walk(segments[i].first);
    if (chain.point_ids.size() >= 2) chains.push_back(std::move(chain));
  }
  return chains;
}

/**
 * Containment test used for outer / hole nesting.
 * Slicing can produce contours that touch each other, so a single vertex test is unreliable:
 * sample several edge midpoints and take the majority vote.
 */
bool contourContainsContour(const QPolygonF &outer,
                            const QRectF &outer_bbox,
                            const QPolygonF &inner,
                            double tolerance) {
  if (inner.size() < 2) return false;
  if (!outer_bbox.adjusted(-tolerance, -tolerance, tolerance, tolerance)
           .contains(inner.boundingRect())) {
    return false;
  }
  const int n = inner.size();
  const int step = std::max(1, n / 5);
  int inside = 0;
  int tested = 0;
  for (int i = 0; i < n; i += step) {
    const QPointF &a = inner[i];
    const QPointF &b = inner[(i + 1) % n];
    const QPointF mid((a.x() + b.x()) * 0.5, (a.y() + b.y()) * 0.5);
    if (outer.containsPoint(mid, Qt::OddEvenFill)) ++inside;
    ++tested;
  }
  return tested > 0 && inside * 2 > tested;
}

/** Fill orientation / parent_index / depth and normalise winding: outer CCW, hole CW. */
void classifyContours(QVector<Contour> &contours, double tolerance) {
  const int count = contours.size();
  if (count == 0) return;

  std::vector<QRectF> bboxes(static_cast<size_t>(count));
  std::vector<double> areas(static_cast<size_t>(count));
  for (int i = 0; i < count; ++i) {
    bboxes[static_cast<size_t>(i)] = contours[i].polygon.boundingRect();
    areas[static_cast<size_t>(i)] = std::abs(signedArea(contours[i].polygon));
  }

  std::vector<std::vector<int>> containers(static_cast<size_t>(count));
  for (int i = 0; i < count; ++i) {
    if (!contours[i].is_closed) continue;
    for (int j = 0; j < count; ++j) {
      if (i == j || !contours[j].is_closed) continue;
      if (contourContainsContour(contours[j].polygon, bboxes[static_cast<size_t>(j)],
                                 contours[i].polygon, tolerance)) {
        containers[static_cast<size_t>(i)].push_back(j);
      }
    }
  }

  for (int i = 0; i < count; ++i) {
    Contour &c = contours[i];
    if (!c.is_closed) {
      c.depth = 0;
      c.parent_index = -1;
      c.orientation = 0;
      continue;
    }
    const std::vector<int> &list = containers[static_cast<size_t>(i)];
    c.depth = static_cast<int>(list.size());
    // The direct parent is the smallest enclosing contour.
    int parent = -1;
    double parent_area = 0.0;
    for (int j : list) {
      const double area = areas[static_cast<size_t>(j)];
      if (parent < 0 || area < parent_area) {
        parent = j;
        parent_area = area;
      }
    }
    c.parent_index = parent;

    const int expected = (c.depth % 2 == 0) ? 1 : -1;
    const double area = signedArea(c.polygon);
    const int actual = (area > 0.0) ? 1 : ((area < 0.0) ? -1 : 0);
    if (actual != 0 && actual != expected) {
      std::reverse(c.polygon.begin(), c.polygon.end());
    }
    c.orientation = expected;
  }
}

Vec3 subtract(const Vec3 &a, const Vec3 &b) {
  return Vec3{a.x - b.x, a.y - b.y, a.z - b.z};
}

Vec3 add(const Vec3 &a, const Vec3 &b) {
  return Vec3{a.x + b.x, a.y + b.y, a.z + b.z};
}

Vec3 multiply(const Vec3 &v, double scale) {
  return Vec3{v.x * scale, v.y * scale, v.z * scale};
}

double dot(const Vec3 &a, const Vec3 &b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 cross(const Vec3 &a, const Vec3 &b) {
  return Vec3{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
              a.x * b.y - a.y * b.x};
}

double lengthSquared(const Vec3 &v) {
  return dot(v, v);
}

double length(const Vec3 &v) {
  return std::sqrt(lengthSquared(v));
}

Vec3 normalised(const Vec3 &v) {
  const double magnitude = length(v);
  return magnitude > 0.0 ? multiply(v, 1.0 / magnitude) : Vec3{};
}

/** SplitMix64 has a fully specified integer sequence on every supported target. */
class FixedRandom {
 public:
  explicit FixedRandom(std::uint64_t stream)
      : state_(kBlueNoiseSeed + UINT64_C(0x9e3779b97f4a7c15) * (stream + 1)) {}

  std::uint64_t next() {
    std::uint64_t z = (state_ += UINT64_C(0x9e3779b97f4a7c15));
    z = (z ^ (z >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94d049bb133111eb);
    return z ^ (z >> 31);
  }

  double unit() {
    // The top 53 bits map exactly onto the binary64 mantissa; no library distribution is involved.
    return static_cast<double>(next() >> 11) * (1.0 / 9007199254740992.0);
  }

 private:
  std::uint64_t state_;
};

struct SamplingTriangle {
  const Triangle *triangle = nullptr;
  double cumulative_area = 0.0;
};

struct SampleBatch {
  std::vector<Vec3> points;
  qint64 candidate_count = 0;
  bool cancelled = false;
};

using Cell3 = std::tuple<qint64, qint64, qint64>;

struct Cell3Hash {
  size_t operator()(const Cell3 &cell) const {
    auto mix = [](std::uint64_t value) {
      value = (value ^ (value >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
      value = (value ^ (value >> 27)) * UINT64_C(0x94d049bb133111eb);
      return value ^ (value >> 31);
    };
    std::uint64_t hash = mix(static_cast<std::uint64_t>(std::get<0>(cell)));
    hash ^= mix(static_cast<std::uint64_t>(std::get<1>(cell)) + UINT64_C(0x9e3779b97f4a7c15));
    hash ^= mix(static_cast<std::uint64_t>(std::get<2>(cell)) + UINT64_C(0x3c6ef372fe94f82a));
    return static_cast<size_t>(hash);
  }
};

SampleBatch sampleBlueNoiseOnPreparedMesh(const Mesh &mesh, double spacing,
                                          std::uint64_t stream,
                                          const ProgressCallback &progress) {
  SampleBatch result;
  std::vector<SamplingTriangle> triangles;
  triangles.reserve(mesh.triangles.size());
  double total_area = 0.0;
  for (size_t triangle_index = 0; triangle_index < mesh.triangles.size(); ++triangle_index) {
    if ((triangle_index & 4095) == 0 && progress &&
        !progress(mesh.triangles.empty()
                      ? 0.1
                      : 0.1 * static_cast<double>(triangle_index) / mesh.triangles.size())) {
      result.cancelled = true;
      return result;
    }
    const Triangle &triangle = mesh.triangles[triangle_index];
    const double area = length(cross(subtract(triangle.v[1], triangle.v[0]),
                                     subtract(triangle.v[2], triangle.v[0]))) *
                        0.5;
    if (!(area > 0.0) || !std::isfinite(area)) continue;
    total_area += area;
    triangles.push_back(SamplingTriangle{&triangle, total_area});
  }
  if (triangles.empty() || !(total_area > 0.0)) return result;

  // Random sequential adsorption settles near 0.7 samples / spacing^2 on a plane. A generous
  // candidate budget lets the set approach saturation while remaining linear in the output size.
  const double estimate_double = std::ceil(total_area * 0.72 / (spacing * spacing));
  if (!std::isfinite(estimate_double) ||
      estimate_double > static_cast<double>(result.points.max_size())) {
    return result;
  }
  const std::uint64_t estimate = static_cast<std::uint64_t>(std::max(1.0, estimate_double));
  if (estimate > std::numeric_limits<std::uint64_t>::max() / 16) return result;
  const std::uint64_t candidate_budget = std::max<std::uint64_t>(128, estimate * 16);
  if (estimate <= result.points.max_size()) {
    result.points.reserve(static_cast<size_t>(estimate));
  }

  FixedRandom random(stream);
  std::unordered_map<Cell3, std::vector<size_t>, Cell3Hash> cells;
  cells.reserve(static_cast<size_t>(estimate));
  const double inverse_cell = 1.0 / spacing;
  const double minimum_distance_squared = spacing * spacing;
  for (std::uint64_t candidate_index = 0; candidate_index < candidate_budget;
       ++candidate_index) {
    if ((candidate_index & 4095) == 0 && progress &&
        !progress(0.1 + 0.9 * static_cast<double>(candidate_index) / candidate_budget)) {
      result.cancelled = true;
      return result;
    }
    const double area_pick = random.unit() * total_area;
    const auto triangle_it = std::lower_bound(
        triangles.begin(), triangles.end(), area_pick,
        [](const SamplingTriangle &entry, double area) { return entry.cumulative_area < area; });
    const Triangle &triangle = *(triangle_it == triangles.end() ? triangles.back().triangle
                                                                 : triangle_it->triangle);

    // Uniform triangle sampling, not uniform barycentric coordinates (which bias the centre).
    const double root = std::sqrt(random.unit());
    const double second = random.unit();
    const double wa = 1.0 - root;
    const double wb = root * (1.0 - second);
    const double wc = root * second;
    const Vec3 candidate{
        triangle.v[0].x * wa + triangle.v[1].x * wb + triangle.v[2].x * wc,
        triangle.v[0].y * wa + triangle.v[1].y * wb + triangle.v[2].y * wc,
        triangle.v[0].z * wa + triangle.v[1].z * wb + triangle.v[2].z * wc,
    };
    const qint64 cell_x = static_cast<qint64>(std::floor(candidate.x * inverse_cell));
    const qint64 cell_y = static_cast<qint64>(std::floor(candidate.y * inverse_cell));
    const qint64 cell_z = static_cast<qint64>(std::floor(candidate.z * inverse_cell));
    bool accepted = true;
    for (qint64 dx = -1; dx <= 1 && accepted; ++dx) {
      for (qint64 dy = -1; dy <= 1 && accepted; ++dy) {
        for (qint64 dz = -1; dz <= 1 && accepted; ++dz) {
          const auto cell_it = cells.find(Cell3{cell_x + dx, cell_y + dy, cell_z + dz});
          if (cell_it == cells.end()) continue;
          for (size_t point_index : cell_it->second) {
            if (lengthSquared(subtract(candidate, result.points[point_index])) + 1e-18 <
                minimum_distance_squared) {
              accepted = false;
              break;
            }
          }
        }
      }
    }
    if (!accepted) continue;
    cells[Cell3{cell_x, cell_y, cell_z}].push_back(result.points.size());
    result.points.push_back(candidate);
  }
  result.candidate_count = static_cast<qint64>(
      std::min<std::uint64_t>(candidate_budget, std::numeric_limits<qint64>::max()));
  if (progress && !progress(1.0)) result.cancelled = true;
  return result;
}

/**
 * Weld STL vertex occurrences and derive one stable inward displacement per welded vertex.
 * Moving a vertex by `displacement * shell_depth` keeps neighbouring triangles connected. The
 * average incident plane moves by approximately shell_depth; this is exact for regular box corners.
 */
class InsetMeshBuilder {
 public:
  bool prepare(const Mesh &mesh, const ProgressCallback &progress = {}) {
    source_ = &mesh;
    Vec3 lo;
    Vec3 hi;
    if (!mesh.boundingBox(&lo, &hi)) return false;
    bounds_min_ = lo;
    bounds_max_ = hi;
    const double diagonal = length(subtract(hi, lo));
    const double tolerance = std::max(1e-10, diagonal * 1e-9);
    const double inverse_cell = 1.0 / tolerance;
    const double tolerance_squared = tolerance * tolerance;
    std::map<Cell3, std::vector<int>> cells;
    triangle_vertices_.resize(mesh.triangles.size());

    for (size_t triangle_index = 0; triangle_index < mesh.triangles.size(); ++triangle_index) {
      if ((triangle_index & 4095) == 0 && progress &&
          !progress(mesh.triangles.empty()
                        ? 0.25
                        : 0.25 * static_cast<double>(triangle_index) / mesh.triangles.size())) {
        return false;
      }
      for (int corner = 0; corner < 3; ++corner) {
        const Vec3 &position = mesh.triangles[triangle_index].v[corner];
        const qint64 cell_x = static_cast<qint64>(std::floor(position.x * inverse_cell));
        const qint64 cell_y = static_cast<qint64>(std::floor(position.y * inverse_cell));
        const qint64 cell_z = static_cast<qint64>(std::floor(position.z * inverse_cell));
        int vertex_id = -1;
        for (qint64 dx = -1; dx <= 1 && vertex_id < 0; ++dx) {
          for (qint64 dy = -1; dy <= 1 && vertex_id < 0; ++dy) {
            for (qint64 dz = -1; dz <= 1 && vertex_id < 0; ++dz) {
              const auto cell_it = cells.find(Cell3{cell_x + dx, cell_y + dy, cell_z + dz});
              if (cell_it == cells.end()) continue;
              for (int candidate : cell_it->second) {
                if (lengthSquared(subtract(position, vertices_[static_cast<size_t>(candidate)])) <=
                    tolerance_squared) {
                  vertex_id = candidate;
                  break;
                }
              }
            }
          }
        }
        if (vertex_id < 0) {
          vertex_id = static_cast<int>(vertices_.size());
          vertices_.push_back(position);
          cells[Cell3{cell_x, cell_y, cell_z}].push_back(vertex_id);
        }
        triangle_vertices_[triangle_index][static_cast<size_t>(corner)] = vertex_id;
      }
    }

    double signed_volume_six = 0.0;
    for (size_t triangle_index = 0; triangle_index < mesh.triangles.size(); ++triangle_index) {
      if ((triangle_index & 4095) == 0 && progress &&
          !progress(0.25 + 0.25 * static_cast<double>(triangle_index) /
                               mesh.triangles.size())) {
        return false;
      }
      const Triangle &triangle = mesh.triangles[triangle_index];
      signed_volume_six += dot(triangle.v[0], cross(triangle.v[1], triangle.v[2]));
    }
    const double volume_epsilon = std::max(1e-20, diagonal * diagonal * diagonal * 1e-12);
    const bool reliable_winding = std::abs(signed_volume_six) > volume_epsilon;
    const double winding_sign = signed_volume_six >= 0.0 ? 1.0 : -1.0;
    const Vec3 bounds_center = multiply(add(lo, hi), 0.5);
    std::vector<std::vector<Vec3>> incident_normals(vertices_.size());
    for (size_t triangle_index = 0; triangle_index < mesh.triangles.size(); ++triangle_index) {
      if ((triangle_index & 4095) == 0 && progress &&
          !progress(0.5 + 0.25 * static_cast<double>(triangle_index) /
                              mesh.triangles.size())) {
        return false;
      }
      const Triangle &triangle = mesh.triangles[triangle_index];
      Vec3 face_normal = normalised(cross(subtract(triangle.v[1], triangle.v[0]),
                                          subtract(triangle.v[2], triangle.v[0])));
      if (lengthSquared(face_normal) == 0.0) continue;
      if (reliable_winding) {
        face_normal = multiply(face_normal, winding_sign);
      } else {
        const Vec3 face_center = multiply(add(add(triangle.v[0], triangle.v[1]), triangle.v[2]),
                                            1.0 / 3.0);
        if (dot(face_normal, subtract(face_center, bounds_center)) < 0.0) {
          face_normal = multiply(face_normal, -1.0);
        }
      }
      for (int corner = 0; corner < 3; ++corner) {
        std::vector<Vec3> &normals = incident_normals[static_cast<size_t>(
            triangle_vertices_[triangle_index][static_cast<size_t>(corner)])];
        bool duplicate = false;
        for (const Vec3 &normal : normals) {
          if (dot(normal, face_normal) > 1.0 - 1e-8) {
            duplicate = true;
            break;
          }
        }
        if (!duplicate) normals.push_back(face_normal);
      }
    }

    displacement_per_unit_.resize(vertices_.size());
    for (size_t vertex_index = 0; vertex_index < vertices_.size(); ++vertex_index) {
      if ((vertex_index & 4095) == 0 && progress &&
          !progress(0.75 + 0.25 * static_cast<double>(vertex_index) /
                               std::max<size_t>(1, vertices_.size()))) {
        return false;
      }
      Vec3 outward_sum;
      for (const Vec3 &normal : incident_normals[vertex_index]) {
        outward_sum = add(outward_sum, normal);
      }
      const Vec3 outward = normalised(outward_sum);
      if (lengthSquared(outward) == 0.0) continue;
      double denominator_sum = 0.0;
      int denominator_count = 0;
      for (const Vec3 &normal : incident_normals[vertex_index]) {
        const double projection = dot(outward, normal);
        if (projection > 1e-6) {
          denominator_sum += projection;
          ++denominator_count;
        }
      }
      if (denominator_count == 0) continue;
      const double denominator = denominator_sum / denominator_count;
      // Very sharp/non-manifold vertices can make the average-plane construction explode. Four
      // times the requested inset is a conservative bevel instead of an unbounded spike.
      const double scale = std::min(4.0, 1.0 / denominator);
      displacement_per_unit_[vertex_index] = multiply(outward, -scale);
    }
    return !progress || progress(1.0);
  }

  Mesh atDepth(double depth, const ProgressCallback &progress = {}) const {
    if (!(depth > 0.0)) return *source_;
    Mesh result;
    result.triangles.resize(triangle_vertices_.size());
    for (size_t triangle_index = 0; triangle_index < triangle_vertices_.size(); ++triangle_index) {
      if ((triangle_index & 4095) == 0 && progress &&
          !progress(triangle_vertices_.empty()
                        ? 1.0
                        : static_cast<double>(triangle_index) / triangle_vertices_.size())) {
        return {};
      }
      for (int corner = 0; corner < 3; ++corner) {
        const int vertex_id = triangle_vertices_[triangle_index][static_cast<size_t>(corner)];
        result.triangles[triangle_index].v[corner] =
            add(vertices_[static_cast<size_t>(vertex_id)],
                multiply(displacement_per_unit_[static_cast<size_t>(vertex_id)], depth));
      }
    }
    if (progress && !progress(1.0)) return {};
    return result;
  }

  double maximumDepth() const {
    const Vec3 extent = subtract(bounds_max_, bounds_min_);
    return std::max(0.0, std::min({extent.x, extent.y, extent.z}) * 0.5);
  }

 private:
  const Mesh *source_ = nullptr;
  Vec3 bounds_min_;
  Vec3 bounds_max_;
  std::vector<Vec3> vertices_;
  std::vector<Vec3> displacement_per_unit_;
  std::vector<std::array<int, 3>> triangle_vertices_;
};

/**
 * A removable, balanced 2D k-d tree used by greedy dot travel planning.
 *
 * Bounding boxes stay static after a point is removed, while `remaining` lets searches discard an
 * exhausted subtree. This keeps removal cheap and nearest-neighbour queries exact.
 */
class NearestPointKdTree {
 public:
  explicit NearestPointKdTree(const QPolygonF &points) : points_(points) {
    nodes_.reserve(points.size());
    point_to_node_.resize(points.size());
    std::fill(point_to_node_.begin(), point_to_node_.end(), -1);
    std::vector<int> indices(static_cast<size_t>(points.size()));
    for (int i = 0; i < points.size(); ++i) indices[static_cast<size_t>(i)] = i;
    root_ = build(indices, 0, static_cast<int>(indices.size()), 0, -1);
  }

  int nearest(const QPointF &anchor) const {
    int best_point = -1;
    double best_distance_squared = std::numeric_limits<double>::infinity();
    search(root_, anchor, &best_point, &best_distance_squared);
    return best_point;
  }

  void remove(int point_index) {
    if (point_index < 0 || point_index >= point_to_node_.size()) return;
    int node_index = point_to_node_[point_index];
    if (node_index < 0 || nodes_[node_index].used) return;
    nodes_[node_index].used = true;
    while (node_index >= 0) {
      --nodes_[node_index].remaining;
      node_index = nodes_[node_index].parent;
    }
  }

 private:
  struct Node {
    int point = -1;
    int left = -1;
    int right = -1;
    int parent = -1;
    int remaining = 0;
    double min_x = 0.0;
    double max_x = 0.0;
    double min_y = 0.0;
    double max_y = 0.0;
    bool used = false;
  };

  int build(std::vector<int> &indices, int begin, int end, int depth, int parent) {
    if (begin >= end) return -1;
    const int middle = begin + (end - begin) / 2;
    const int axis = depth & 1;
    std::nth_element(indices.begin() + begin, indices.begin() + middle, indices.begin() + end,
                     [&](int a, int b) {
                       const QPointF &pa = points_[a];
                       const QPointF &pb = points_[b];
                       const double primary_a = axis == 0 ? pa.x() : pa.y();
                       const double primary_b = axis == 0 ? pb.x() : pb.y();
                       if (primary_a != primary_b) return primary_a < primary_b;
                       const double secondary_a = axis == 0 ? pa.y() : pa.x();
                       const double secondary_b = axis == 0 ? pb.y() : pb.x();
                       if (secondary_a != secondary_b) return secondary_a < secondary_b;
                       return a < b;
                     });

    const int point_index = indices[static_cast<size_t>(middle)];
    const int node_index = nodes_.size();
    nodes_.append(Node{});
    point_to_node_[point_index] = node_index;
    const int left = build(indices, begin, middle, depth + 1, node_index);
    const int right = build(indices, middle + 1, end, depth + 1, node_index);

    Node &node = nodes_[node_index];
    node.point = point_index;
    node.left = left;
    node.right = right;
    node.parent = parent;
    node.remaining = 1;
    const QPointF &point = points_[point_index];
    node.min_x = node.max_x = point.x();
    node.min_y = node.max_y = point.y();
    includeChildBounds(left, &node);
    includeChildBounds(right, &node);
    return node_index;
  }

  void includeChildBounds(int child_index, Node *node) {
    if (child_index < 0) return;
    const Node &child = nodes_[child_index];
    node->remaining += child.remaining;
    node->min_x = std::min(node->min_x, child.min_x);
    node->max_x = std::max(node->max_x, child.max_x);
    node->min_y = std::min(node->min_y, child.min_y);
    node->max_y = std::max(node->max_y, child.max_y);
  }

  double lowerBoundSquared(int node_index, const QPointF &anchor) const {
    if (node_index < 0 || nodes_[node_index].remaining == 0) {
      return std::numeric_limits<double>::infinity();
    }
    const Node &node = nodes_[node_index];
    const double dx = anchor.x() < node.min_x ? node.min_x - anchor.x()
                      : anchor.x() > node.max_x ? anchor.x() - node.max_x
                                                : 0.0;
    const double dy = anchor.y() < node.min_y ? node.min_y - anchor.y()
                      : anchor.y() > node.max_y ? anchor.y() - node.max_y
                                                : 0.0;
    return dx * dx + dy * dy;
  }

  void search(int node_index, const QPointF &anchor, int *best_point,
              double *best_distance_squared) const {
    if (node_index < 0 || nodes_[node_index].remaining == 0 ||
        lowerBoundSquared(node_index, anchor) > *best_distance_squared) {
      return;
    }
    const Node &node = nodes_[node_index];
    if (!node.used) {
      const QPointF delta = points_[node.point] - anchor;
      const double distance_squared = delta.x() * delta.x() + delta.y() * delta.y();
      if (*best_point < 0 || distance_squared < *best_distance_squared ||
          (distance_squared == *best_distance_squared && node.point < *best_point)) {
        *best_point = node.point;
        *best_distance_squared = distance_squared;
      }
    }

    int first = node.left;
    int second = node.right;
    double first_bound = lowerBoundSquared(first, anchor);
    double second_bound = lowerBoundSquared(second, anchor);
    if (second_bound < first_bound || (second_bound == first_bound && second < first)) {
      std::swap(first, second);
      std::swap(first_bound, second_bound);
    }
    if (first_bound <= *best_distance_squared) {
      search(first, anchor, best_point, best_distance_squared);
    }
    if (second_bound <= *best_distance_squared) {
      search(second, anchor, best_point, best_distance_squared);
    }
  }

  const QPolygonF &points_;
  QVector<Node> nodes_;
  QVector<int> point_to_node_;
  int root_ = -1;
};

bool parseBinary(const QByteArray &data, Mesh *mesh, QString *error) {
  const quint32 face_count = readUint32LE(data.constData() + 80);
  if (face_count == 0) {
    if (error != nullptr) *error = QStringLiteral("binary STL declares 0 triangles");
    return false;
  }
  mesh->triangles.clear();
  mesh->triangles.reserve(face_count);
  const char *p = data.constData() + kBinaryHeaderSize;
  for (quint32 i = 0; i < face_count; ++i) {
    // 12 bytes normal (ignored -- exporters routinely write wrong normals), 3 * 12 bytes vertices,
    // 2 bytes attribute count.
    const char *v = p + 12;
    Triangle tri;
    for (int k = 0; k < 3; ++k) {
      tri.v[k].x = readFloatLE(v + k * 12 + 0);
      tri.v[k].y = readFloatLE(v + k * 12 + 4);
      tri.v[k].z = readFloatLE(v + k * 12 + 8);
    }
    mesh->triangles.push_back(tri);
    p += kBinaryTriangleSize;
  }
  return true;
}

bool parseAscii(const QByteArray &data, Mesh *mesh, QString *error) {
  mesh->triangles.clear();
  const char *p = data.constData();
  const char *end = p + data.size();
  Triangle tri;
  int vertex_index = 0;
  while (p < end) {
    while (p < end && isAsciiSpace(*p)) ++p;
    const char *token = p;
    while (p < end && !isAsciiSpace(*p)) ++p;
    const qsizetype length = p - token;
    if (length != 6 || std::strncmp(token, "vertex", 6) != 0) continue;

    double coord[3] = {0.0, 0.0, 0.0};
    for (int k = 0; k < 3; ++k) {
      while (p < end && isAsciiSpace(*p)) ++p;
      char *number_end = nullptr;
      coord[k] = std::strtod(p, &number_end);
      if (number_end == p) {
        if (error != nullptr) *error = QStringLiteral("malformed vertex in ASCII STL");
        return false;
      }
      p = number_end;
    }
    tri.v[vertex_index].x = coord[0];
    tri.v[vertex_index].y = coord[1];
    tri.v[vertex_index].z = coord[2];
    ++vertex_index;
    if (vertex_index == 3) {
      mesh->triangles.push_back(tri);
      vertex_index = 0;
    }
  }
  if (vertex_index != 0) {
    if (error != nullptr) *error = QStringLiteral("truncated ASCII STL: incomplete facet");
    return false;
  }
  if (mesh->triangles.empty()) {
    if (error != nullptr) *error = QStringLiteral("ASCII STL contains no triangle");
    return false;
  }
  return true;
}

}  // namespace

bool Mesh::boundingBox(Vec3 *min_corner, Vec3 *max_corner) const {
  if (triangles.empty()) return false;
  Vec3 lo = triangles.front().v[0];
  Vec3 hi = lo;
  for (const Triangle &tri : triangles) {
    for (int i = 0; i < 3; ++i) {
      lo.x = std::min(lo.x, tri.v[i].x);
      lo.y = std::min(lo.y, tri.v[i].y);
      lo.z = std::min(lo.z, tri.v[i].z);
      hi.x = std::max(hi.x, tri.v[i].x);
      hi.y = std::max(hi.y, tri.v[i].y);
      hi.z = std::max(hi.z, tri.v[i].z);
    }
  }
  if (min_corner != nullptr) *min_corner = lo;
  if (max_corner != nullptr) *max_corner = hi;
  return true;
}

Format detectFormat(const QByteArray &data) {
  if (data.size() >= kBinaryHeaderSize) {
    const quint32 face_count = readUint32LE(data.constData() + 80);
    const qint64 expected = kBinaryHeaderSize + static_cast<qint64>(face_count) * kBinaryTriangleSize;
    if (expected == data.size()) return Format::Binary;
  }
  // Only reached when the binary size identity fails, so a binary file whose 80 byte header happens
  // to contain the word "facet" cannot be misclassified here.
  if (data.left(2048).toLower().contains("facet")) return Format::Ascii;
  return Format::Unknown;
}

bool readMesh(const QByteArray &data, Mesh *mesh, QString *error) {
  if (mesh == nullptr) return false;
  mesh->triangles.clear();
  if (data.isEmpty()) {
    if (error != nullptr) *error = QStringLiteral("empty file");
    return false;
  }
  switch (detectFormat(data)) {
    case Format::Binary:
      return parseBinary(data, mesh, error);
    case Format::Ascii:
      return parseAscii(data, mesh, error);
    case Format::Unknown:
    default:
      break;
  }
  if (error != nullptr) {
    if (data.size() >= kBinaryHeaderSize) {
      const quint32 face_count = readUint32LE(data.constData() + 80);
      const qint64 expected = kBinaryHeaderSize + static_cast<qint64>(face_count) * kBinaryTriangleSize;
      *error = QStringLiteral(
                   "not a valid STL: declared %1 triangles need %2 bytes but the file has %3 bytes "
                   "(truncated binary?), and no ASCII facet keyword was found")
                   .arg(face_count)
                   .arg(expected)
                   .arg(data.size());
    } else {
      *error = QStringLiteral("not a valid STL: file is only %1 bytes").arg(data.size());
    }
  }
  return false;
}

bool readPointCloud(const QByteArray &data, PointCloud *cloud, QString *error) {
  if (cloud == nullptr) return false;
  cloud->points.clear();
  if (data.size() < kPointCloudHeaderSize || data.left(4) != QByteArrayLiteral("BSPC")) {
    if (error != nullptr) *error = QStringLiteral("invalid BSPC header");
    return false;
  }

  const quint8 version = static_cast<quint8>(data[4]);
  const quint8 components = static_cast<quint8>(data[5]);
  const quint32 point_count = readUint32LE(data.constData() + 8);
  const qint64 expected_size =
      kPointCloudHeaderSize + static_cast<qint64>(point_count) * kPointCloudPositionSize;
  if (version != 1 || components != 3 || point_count == 0 || expected_size != data.size()) {
    if (error != nullptr) {
      *error = QStringLiteral("unsupported or malformed BSPC: version %1, components %2, "
                              "points %3, bytes %4 (expected %5)")
                   .arg(version)
                   .arg(components)
                   .arg(point_count)
                   .arg(data.size())
                   .arg(expected_size);
    }
    return false;
  }

  cloud->points.reserve(point_count);
  const char *cursor = data.constData() + kPointCloudHeaderSize;
  for (quint32 index = 0; index < point_count; ++index) {
    Vec3 point{readFloatLE(cursor), readFloatLE(cursor + 4), readFloatLE(cursor + 8)};
    if (!std::isfinite(point.x) || !std::isfinite(point.y) || !std::isfinite(point.z)) {
      cloud->points.clear();
      if (error != nullptr) *error = QStringLiteral("BSPC contains a non-finite position");
      return false;
    }
    cloud->points.push_back(point);
    cursor += kPointCloudPositionSize;
  }
  return true;
}

void applyTransform(Mesh *mesh, const QMatrix4x4 &transform) {
  if (mesh == nullptr || transform.isIdentity()) return;
  const float *m = transform.constData();  // column major
  for (Triangle &tri : mesh->triangles) {
    for (int i = 0; i < 3; ++i) {
      const Vec3 s = tri.v[i];
      const double w = static_cast<double>(m[3]) * s.x + static_cast<double>(m[7]) * s.y +
                       static_cast<double>(m[11]) * s.z + static_cast<double>(m[15]);
      const double inv_w = (std::abs(w) > 1e-12) ? 1.0 / w : 1.0;
      tri.v[i].x = (static_cast<double>(m[0]) * s.x + static_cast<double>(m[4]) * s.y +
                    static_cast<double>(m[8]) * s.z + static_cast<double>(m[12])) * inv_w;
      tri.v[i].y = (static_cast<double>(m[1]) * s.x + static_cast<double>(m[5]) * s.y +
                    static_cast<double>(m[9]) * s.z + static_cast<double>(m[13])) * inv_w;
      tri.v[i].z = (static_cast<double>(m[2]) * s.x + static_cast<double>(m[6]) * s.y +
                    static_cast<double>(m[10]) * s.z + static_cast<double>(m[14])) * inv_w;
    }
  }
}

PointCloudResult sampleSurfaceBlueNoise(const Mesh &mesh, const QMatrix4x4 &transform,
                                        const BlueNoiseParams &params,
                                        ProgressCallback progress) {
  PointCloudResult result;
  if (mesh.isEmpty()) {
    result.error = QStringLiteral("mesh has no triangle");
    return result;
  }
  if (!(params.spacing > 0.0) || !std::isfinite(params.spacing)) {
    result.error = QStringLiteral("point spacing must be finite and greater than 0");
    return result;
  }

  Mesh transformed = mesh;
  applyTransform(&transformed, transform);
  if (progress && !progress(0.05)) {
    result.error = QStringLiteral("cancelled");
    return result;
  }
  SampleBatch batch = sampleBlueNoiseOnPreparedMesh(
      transformed, params.spacing, 0,
      [&](double value) { return !progress || progress(0.05 + 0.95 * value); });
  if (batch.cancelled) {
    result.error = QStringLiteral("cancelled");
    return result;
  }
  if (batch.points.empty()) {
    result.error = QStringLiteral("transformed mesh has no non-degenerate surface");
    return result;
  }
  result.points.reserve(batch.points.size());
  for (const Vec3 &point : batch.points) result.points.push_back(SurfacePoint{point, 0});
  result.shell_count = 1;
  result.candidate_count = batch.candidate_count;
  result.ok = true;
  return result;
}

PointCloudResult sampleInwardShellsBlueNoise(const Mesh &mesh, const QMatrix4x4 &transform,
                                             const BlueNoiseParams &params,
                                             ProgressCallback progress) {
  PointCloudResult result;
  if (mesh.isEmpty()) {
    result.error = QStringLiteral("mesh has no triangle");
    return result;
  }
  if (!(params.spacing > 0.0) || !std::isfinite(params.spacing)) {
    result.error = QStringLiteral("point spacing must be finite and greater than 0");
    return result;
  }
  const double shell_spacing = params.shell_spacing > 0.0 ? params.shell_spacing : params.spacing;
  if (!std::isfinite(shell_spacing)) {
    result.error = QStringLiteral("shell spacing must be finite");
    return result;
  }
  if (params.max_shell_count <= 0) {
    result.error = QStringLiteral("maximum shell count must be greater than 0");
    return result;
  }

  // This copy is the coordinate-space boundary: everything below, including vertex welding,
  // winding, normals, bounds, inset distance and Poisson distances, sees transformed geometry.
  Mesh transformed = mesh;
  applyTransform(&transformed, transform);
  if (progress && !progress(0.02)) {
    result.error = QStringLiteral("cancelled");
    return result;
  }
  InsetMeshBuilder builder;
  bool cancelled = false;
  if (!builder.prepare(transformed, [&](double value) {
        const bool keep_going = !progress || progress(0.02 + 0.18 * value);
        cancelled = !keep_going;
        return keep_going;
      })) {
    result.error = cancelled ? QStringLiteral("cancelled")
                             : QStringLiteral("cannot prepare transformed mesh for inward shells");
    return result;
  }

  const double maximum_depth = builder.maximumDepth();
  const double geometric_shell_count =
      maximum_depth > 0.0 ? std::max(1.0, std::ceil(maximum_depth / shell_spacing)) : 1.0;
  const int shell_count = geometric_shell_count > params.max_shell_count
                              ? params.max_shell_count
                              : static_cast<int>(geometric_shell_count);
  result.shell_limit_reached = geometric_shell_count > params.max_shell_count;
  for (int shell_index = 0; shell_index < shell_count; ++shell_index) {
    const double shell_start = 0.2 + 0.8 * static_cast<double>(shell_index) / shell_count;
    const double shell_span = 0.8 / shell_count;
    const double depth = shell_index * shell_spacing;
    if (shell_index > 0 && depth >= maximum_depth) break;
    const Mesh shell = builder.atDepth(depth, [&](double value) {
      const bool keep_going = !progress || progress(shell_start + shell_span * 0.15 * value);
      cancelled = !keep_going;
      return keep_going;
    });
    if (cancelled) {
      result.error = QStringLiteral("cancelled");
      return result;
    }
    // Stream 1+shell keeps every shell different while preserving stream 0 for plain surface mode.
    SampleBatch batch = sampleBlueNoiseOnPreparedMesh(
        shell, params.spacing, static_cast<std::uint64_t>(shell_index) + 1,
        [&](double value) {
          return !progress || progress(shell_start + shell_span * (0.15 + 0.85 * value));
        });
    if (batch.cancelled) {
      result.error = QStringLiteral("cancelled");
      return result;
    }
    if (batch.points.empty()) {
      if (shell_index == 0) {
        result.error = QStringLiteral("transformed mesh has no non-degenerate surface");
        return result;
      }
      break;
    }
    if (result.candidate_count <= std::numeric_limits<qint64>::max() - batch.candidate_count) {
      result.candidate_count += batch.candidate_count;
    } else {
      result.candidate_count = std::numeric_limits<qint64>::max();
    }
    result.points.reserve(result.points.size() + batch.points.size());
    for (const Vec3 &point : batch.points) {
      result.points.push_back(SurfacePoint{point, shell_index});
    }
    ++result.shell_count;
  }
  result.ok = !result.points.empty();
  if (!result.ok && result.error.isEmpty()) result.error = QStringLiteral("no shell point generated");
  return result;
}

double signedArea(const QPolygonF &polygon) {
  const int n = polygon.size();
  if (n < 3) return 0.0;
  double sum = 0.0;
  for (int i = 0; i < n; ++i) {
    const QPointF &a = polygon[i];
    const QPointF &b = polygon[(i + 1) % n];
    sum += a.x() * b.y() - b.x() * a.y();
  }
  return sum * 0.5;
}

QVector<int> nearestPointOrder(const QPolygonF &points, const QPointF &anchor,
                               ProgressCallback progress) {
  QVector<int> result;
  result.reserve(points.size());
  NearestPointKdTree tree(points);
  QPointF cursor = anchor;
  for (int i = 0; i < points.size(); ++i) {
    if ((i & 1023) == 0 && progress &&
        !progress(points.isEmpty() ? 1.0 : static_cast<double>(i) / points.size())) {
      return {};
    }
    const int point_index = tree.nearest(cursor);
    if (point_index < 0) break;
    result.append(point_index);
    tree.remove(point_index);
    cursor = points[point_index];
  }
  if (progress) progress(1.0);
  return result;
}

bool Slicer::prepare(const Mesh &mesh, const QMatrix4x4 &transform, const SliceParams &params,
                     QString *error, ProgressCallback progress) {
  ready_ = false;
  owns_mesh_ = false;
  source_ = nullptr;
  transformed_.triangles.clear();
  tri_min_z_.clear();
  tri_max_z_.clear();
  tri_xy_rate_.clear();
  order_.clear();
  active_.clear();
  next_triangle_ = 0;
  swept_ = false;

  if (mesh.isEmpty()) {
    if (error != nullptr) *error = QStringLiteral("mesh has no triangle");
    return false;
  }
  if (!(params.layer_height > 0.0)) {
    if (error != nullptr) *error = QStringLiteral("layer height must be greater than 0");
    return false;
  }
  params_ = params;

  // Only copy the mesh when there is something to do to it: a 700k triangle model is ~50MB.
  if (transform.isIdentity()) {
    source_ = &mesh;
    owns_mesh_ = false;
  } else {
    transformed_ = mesh;
    if (!transform.isIdentity()) applyTransform(&transformed_, transform);
    owns_mesh_ = true;
  }
  if (progress && !progress(0.05)) {
    if (error != nullptr) *error = QStringLiteral("cancelled");
    return false;
  }

  const Mesh &mesh_ref = work();
  Vec3 lo;
  Vec3 hi;
  mesh_ref.boundingBox(&lo, &hi);
  min_z_ = lo.z;
  max_z_ = hi.z;
  const double dx = hi.x - lo.x;
  const double dy = hi.y - lo.y;
  const double dz = hi.z - lo.z;
  const double diagonal = std::sqrt(dx * dx + dy * dy + dz * dz);
  // Vertices closer than this to a slicing plane are pushed above it, so no vertex ever lies
  // exactly on a plane.
  plane_eps_ = std::max(1e-12, diagonal * 1e-9);
  weld_tolerance_ =
      (params.weld_tolerance > 0.0) ? params.weld_tolerance : std::max(1e-9, diagonal * 1e-7);
  const double area_eps = std::max(1e-20, diagonal * diagonal * 1e-14);

  const size_t triangle_count = mesh_ref.triangles.size();
  tri_min_z_.resize(triangle_count);
  tri_max_z_.resize(triangle_count);
  tri_xy_rate_.resize(triangle_count, 0.0);
  order_.reserve(triangle_count);
  for (size_t i = 0; i < triangle_count; ++i) {
    if ((i & 4095) == 0 && progress &&
        !progress(0.05 + 0.9 * static_cast<double>(i) / triangle_count)) {
      if (error != nullptr) *error = QStringLiteral("cancelled");
      return false;
    }
    const Triangle &tri = mesh_ref.triangles[i];
    tri_min_z_[i] = std::min({tri.v[0].z, tri.v[1].z, tri.v[2].z});
    tri_max_z_[i] = std::max({tri.v[0].z, tri.v[1].z, tri.v[2].z});
    const double ax = tri.v[1].x - tri.v[0].x;
    const double ay = tri.v[1].y - tri.v[0].y;
    const double az = tri.v[1].z - tri.v[0].z;
    const double bx = tri.v[2].x - tri.v[0].x;
    const double by = tri.v[2].y - tri.v[0].y;
    const double bz = tri.v[2].z - tri.v[0].z;
    const double cx = ay * bz - az * by;
    const double cy = az * bx - ax * bz;
    const double cz = ax * by - ay * bx;
    if (std::sqrt(cx * cx + cy * cy + cz * cz) * 0.5 < area_eps) {
      continue;  // zero area triangle, it can only produce zero length segments
    }
    const double lateral_normal = std::hypot(cx, cy);
    // When Z changes, the plane/triangle intersection moves in XY by |nz|/|nxy|. Truly horizontal
    // triangles have no Z span and therefore do not contribute a contour interval.
    if (tri_max_z_[i] - tri_min_z_[i] > plane_eps_ && lateral_normal > 1e-20) {
      tri_xy_rate_[i] = std::abs(cz) / lateral_normal;
    }
    order_.push_back(static_cast<int>(i));
  }
  std::sort(order_.begin(), order_.end(), [&](int a, int b) {
    const double a_z = tri_min_z_[static_cast<size_t>(a)];
    const double b_z = tri_min_z_[static_cast<size_t>(b)];
    return a_z < b_z || (a_z == b_z && a < b);
  });

  if (progress && !progress(1.0)) {
    if (error != nullptr) *error = QStringLiteral("cancelled");
    return false;
  }
  ready_ = true;
  return true;
}

QVector<double> Slicer::planes() const {
  QVector<double> result;
  if (!isReady()) return result;

  // The topmost plane may land above the mesh and simply produce an empty layer, which is legal.
  int count = static_cast<int>(std::ceil((max_z_ - min_z_) / params_.layer_height));
  if (count < 1) count = 1;  // flat mesh, or layer height larger than the model
  // Half a layer above the bottom so the extreme planes never graze the bbox faces, but never above
  // the middle of the model: with a layer height larger than the model height that offset would
  // push the only plane past the top and silently return nothing.
  const double first =
      std::min(min_z_ + params_.layer_height * 0.5, (min_z_ + max_z_) * 0.5);

  result.reserve(count);
  for (int i = 0; i < count; ++i) result.push_back(first + i * params_.layer_height);
  return result;
}

QVector<double> Slicer::adaptivePlanes(double minimum_layer_height,
                                       ProgressCallback progress) const {
  QVector<double> result;
  if (!isReady()) return result;
  if (!(max_z_ > min_z_)) {
    result.push_back(min_z_);
    return result;
  }

  const double maximum_step = params_.layer_height;
  if (!(minimum_layer_height > 0.0) || !std::isfinite(minimum_layer_height)) {
    return planes();
  }
  const double minimum_step = std::min(minimum_layer_height, maximum_step);
  // Maximum lateral contour movement allowed inside one layer. This makes shallow facets request
  // smaller Z steps while vertical walls keep the user-selected maximum step.
  const double maximum_xy_error = maximum_step * 0.25;
  struct RateEntry {
    double rate = 0.0;
    double maximum_z = 0.0;
    int triangle_index = -1;
  };
  struct LowerRate {
    bool operator()(const RateEntry &a, const RateEntry &b) const {
      return a.rate < b.rate || (a.rate == b.rate && a.triangle_index > b.triangle_index);
    }
  };
  std::priority_queue<RateEntry, std::vector<RateEntry>, LowerRate> active_rates;
  size_t next_triangle = 0;
  double interval_bottom = min_z_;
  int progress_counter = 0;
  while (interval_bottom < max_z_ - plane_eps_) {
    if (((progress_counter++) & 255) == 0 && progress &&
        !progress((interval_bottom - min_z_) / (max_z_ - min_z_))) {
      return {};
    }
    const double probe_top = std::min(max_z_, interval_bottom + maximum_step);
    // probe_top only moves upward, so every triangle enters this planning sweep once. Expired
    // entries are discarded lazily from the max heap, keeping planning O(triangles log triangles)
    // instead of rescanning a 700k-triangle mesh for every Z interval.
    while (next_triangle < order_.size()) {
      const int triangle_index = order_[next_triangle];
      const size_t index = static_cast<size_t>(triangle_index);
      if (tri_min_z_[index] > probe_top) break;
      if (tri_max_z_[index] >= interval_bottom && tri_xy_rate_[index] > 0.0) {
        active_rates.push(RateEntry{tri_xy_rate_[index], tri_max_z_[index], triangle_index});
      }
      ++next_triangle;
    }
    while (!active_rates.empty() && active_rates.top().maximum_z < interval_bottom) {
      active_rates.pop();
    }
    const double maximum_rate = active_rates.empty() ? 0.0 : active_rates.top().rate;
    double step = maximum_step;
    if (maximum_rate > 0.0) step = std::min(step, maximum_xy_error / maximum_rate);
    step = std::max(minimum_step, step);
    step = std::min(step, max_z_ - interval_bottom);
    if (!(step > 0.0) || !std::isfinite(step)) break;
    result.push_back(interval_bottom + step * 0.5);
    interval_bottom += step;
  }
  if (result.isEmpty()) result.push_back((min_z_ + max_z_) * 0.5);
  if (progress) progress(1.0);
  return result;
}

Layer Slicer::sliceAt(double z, ProgressCallback progress) {
  Layer layer;
  layer.z_geometry = z;
  if (!isReady()) return layer;

  const Mesh &mesh_ref = work();

  // The active list is a forward sweep: triangles enter once their minimum Z is reached and leave
  // once their maximum Z is passed. Going back down means the sweep has to start over.
  if (swept_ && z < last_z_) {
    active_.clear();
    next_triangle_ = 0;
  }
  swept_ = true;
  last_z_ = z;

  while (next_triangle_ < order_.size() &&
         tri_min_z_[static_cast<size_t>(order_[next_triangle_])] <= z) {
    active_.push_back(order_[next_triangle_]);
    ++next_triangle_;
  }
  size_t write = 0;
  for (size_t read = 0; read < active_.size(); ++read) {
    if ((read & 4095) == 0 && progress &&
        !progress(active_.empty() ? 0.1
                                 : 0.1 * static_cast<double>(read) / active_.size())) {
      return {};
    }
    if (tri_max_z_[static_cast<size_t>(active_[read])] >= z) active_[write++] = active_[read];
  }
  active_.resize(write);

  std::vector<std::pair<int, int>> segments;
  PointWelder welder(weld_tolerance_);
  for (size_t active_index = 0; active_index < active_.size(); ++active_index) {
    if ((active_index & 4095) == 0 && progress &&
        !progress(0.1 + 0.85 * static_cast<double>(active_index) /
                            std::max<size_t>(1, active_.size()))) {
      return {};
    }
    const int index = active_[active_index];
    const Triangle &tri = mesh_ref.triangles[static_cast<size_t>(index)];
    double d[3];
    int above = 0;
    int on_plane = 0;
    for (int i = 0; i < 3; ++i) {
      d[i] = tri.v[i].z - z;
      if (std::abs(d[i]) < plane_eps_) {
        d[i] = plane_eps_;  // consistent rule: a vertex on the plane counts as above it
        ++on_plane;
      }
      if (d[i] > 0.0) ++above;
    }
    if (on_plane == 3) {
      // Triangle lies in the plane; skip it to avoid double-counting its edges.
      continue;
    }
    if (above == 0 || above == 3) continue;

    // Exactly two edges cross the plane because no vertex sits on it any more.
    QPointF crossing[2];
    int found = 0;
    for (int i = 0; i < 3 && found < 2; ++i) {
      const int j = (i + 1) % 3;
      if ((d[i] > 0.0) == (d[j] > 0.0)) continue;
      const double t = d[i] / (d[i] - d[j]);
      crossing[found++] = QPointF(tri.v[i].x + (tri.v[j].x - tri.v[i].x) * t,
                                  tri.v[i].y + (tri.v[j].y - tri.v[i].y) * t);
    }
    if (found != 2) continue;
    const int a = welder.insert(crossing[0].x(), crossing[0].y());
    const int b = welder.insert(crossing[1].x(), crossing[1].y());
    if (a != b) segments.emplace_back(a, b);
  }

  if (!segments.empty()) {
    const std::vector<QPointF> &points = welder.points();
    const std::vector<Chain> chains = chainSegments(segments, static_cast<int>(points.size()));
    for (const Chain &chain : chains) {
      const size_t unique = chain.closed ? chain.point_ids.size() - 1 : chain.point_ids.size();
      if (unique < 2 || (chain.closed && unique < 3)) continue;
      Contour contour;
      contour.is_closed = chain.closed;
      contour.polygon.reserve(static_cast<int>(chain.point_ids.size()));
      for (int id : chain.point_ids) contour.polygon << points[static_cast<size_t>(id)];
      layer.contours.push_back(std::move(contour));
    }
    classifyContours(layer.contours, weld_tolerance_);
  }

  if (progress) progress(1.0);
  return layer;
}

}  // namespace stl
