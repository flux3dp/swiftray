#include "stl-utils.h"

#include <QElapsedTimer>
#include <QFile>
#include <QHash>
#include <QRectF>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <utility>

namespace stl {

namespace {

constexpr qint64 kBinaryHeaderSize = 84;
constexpr qint64 kBinaryTriangleSize = 50;

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
    return (ix << 32) ^ (iy & 0xffffffffLL);
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
                                 int point_count,
                                 int *non_manifold_junctions) {
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

  if (non_manifold_junctions != nullptr) {
    for (const auto &list : incident) {
      if (list.size() > 2) ++(*non_manifold_junctions);
    }
  }

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
 * Containment test used for the outer / hole nesting (TODO.md B-7).
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

bool readMeshFromFile(const QString &file_path, Mesh *mesh, QString *error) {
  QFile file(file_path);
  if (!file.exists()) {
    if (error != nullptr) *error = QStringLiteral("file not found: %1").arg(file_path);
    return false;
  }
  if (!file.open(QIODevice::ReadOnly)) {
    if (error != nullptr) *error = QStringLiteral("cannot open file: %1").arg(file_path);
    return false;
  }
  const QByteArray data = file.readAll();
  return readMesh(data, mesh, error);
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

QPolygonF resamplePolygon(const QPolygonF &polygon, double spacing, bool is_closed) {
  QPolygonF result;
  if (polygon.isEmpty() || !(spacing > 0.0)) return result;

  QVector<QPointF> path(polygon.begin(), polygon.end());
  // Closed contours are stored with a repeated first point; drop it and re-add it as the closing
  // edge so the walk covers the whole loop exactly once.
  if (is_closed && path.size() > 1 && path.front() == path.back()) path.removeLast();
  if (path.isEmpty()) return result;
  if (is_closed && path.size() > 1) {
    const QPointF start = path.front();  // copy first, appending an element of the same container
    path.push_back(start);
  }
  if (path.size() < 2) {
    result << path.front();
    return result;
  }

  std::vector<double> cumulative(static_cast<size_t>(path.size()), 0.0);
  for (int i = 1; i < path.size(); ++i) {
    const QPointF d = path[i] - path[i - 1];
    cumulative[static_cast<size_t>(i)] =
        cumulative[static_cast<size_t>(i - 1)] + std::sqrt(d.x() * d.x() + d.y() * d.y());
  }
  const double length = cumulative.back();
  if (length <= 0.0) {
    result << path.front();
    return result;
  }

  // Sample positions are k * spacing; anything left over at the end is discarded on purpose.
  const int sample_count = static_cast<int>(std::floor(length / spacing));
  int segment = 1;
  for (int k = 0; k <= sample_count; ++k) {
    const double target = k * spacing;
    // A closed contour must not repeat its start point.
    if (is_closed && k == sample_count && std::abs(length - target) <= spacing * 1e-9) break;
    while (segment < path.size() - 1 && cumulative[static_cast<size_t>(segment)] < target) ++segment;
    const double previous = cumulative[static_cast<size_t>(segment - 1)];
    const double span = cumulative[static_cast<size_t>(segment)] - previous;
    const double t = (span > 0.0) ? (target - previous) / span : 0.0;
    result << path[segment - 1] + (path[segment] - path[segment - 1]) * t;
  }
  if (result.isEmpty()) result << path.front();
  return result;
}

SliceResult sliceMesh(const Mesh &mesh, const QMatrix4x4 &transform, const SliceParams &params) {
  SliceResult result;
  QElapsedTimer timer;
  timer.start();

  if (mesh.isEmpty()) {
    result.error = QStringLiteral("mesh has no triangle");
    return result;
  }
  if (!(params.layer_height > 0.0)) {
    result.error = QStringLiteral("layer height must be greater than 0");
    return result;
  }

  // Only copy the mesh when there is something to transform: a 700k triangle model is ~50MB.
  Mesh transformed;
  if (!transform.isIdentity()) {
    transformed = mesh;
    applyTransform(&transformed, transform);
  }
  const Mesh &work = transform.isIdentity() ? mesh : transformed;

  Vec3 lo;
  Vec3 hi;
  work.boundingBox(&lo, &hi);
  const double dx = hi.x - lo.x;
  const double dy = hi.y - lo.y;
  const double dz = hi.z - lo.z;
  const double diagonal = std::sqrt(dx * dx + dy * dy + dz * dz);
  // Vertices closer than this to a slicing plane are pushed above it, so no vertex ever lies
  // exactly on a plane (TODO.md 5-(e)).
  const double plane_eps = std::max(1e-12, diagonal * 1e-9);
  const double weld_tolerance =
      (params.weld_tolerance > 0.0) ? params.weld_tolerance : std::max(1e-9, diagonal * 1e-7);
  const double area_eps = std::max(1e-20, diagonal * diagonal * 1e-14);

  const size_t triangle_count = work.triangles.size();
  std::vector<double> tri_min_z(triangle_count);
  std::vector<double> tri_max_z(triangle_count);
  std::vector<int> order;
  order.reserve(triangle_count);
  for (size_t i = 0; i < triangle_count; ++i) {
    const Triangle &tri = work.triangles[i];
    tri_min_z[i] = std::min({tri.v[0].z, tri.v[1].z, tri.v[2].z});
    tri_max_z[i] = std::max({tri.v[0].z, tri.v[1].z, tri.v[2].z});
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
      ++result.stats.degenerate_triangle_count;
      continue;  // zero area triangle, it can only produce zero length segments
    }
    order.push_back(static_cast<int>(i));
  }
  std::sort(order.begin(), order.end(),
            [&](int a, int b) { return tri_min_z[static_cast<size_t>(a)] < tri_min_z[static_cast<size_t>(b)]; });

  // Layer count follows the documented formula; the topmost plane may land above the mesh and
  // simply produce an empty layer, which is legal (TODO.md 5-(m)).
  int layer_count = static_cast<int>(std::ceil(dz / params.layer_height));
  if (layer_count < 1) layer_count = 1;  // flat mesh, or layer height larger than the model
  // Half a layer above the bottom so the extreme planes never graze the bbox faces (TODO.md 5-(o)),
  // but never above the middle of the model: with a layer height larger than the model height that
  // offset would push the only plane past the top and silently return an empty result (5-(n)).
  const double first_z = std::min(lo.z + params.layer_height * 0.5, (lo.z + hi.z) * 0.5);

  result.layers.reserve(layer_count);
  std::vector<int> active;
  size_t next_triangle = 0;
  std::vector<std::pair<int, int>> segments;
  PointWelder welder(weld_tolerance);

  QElapsedTimer layer_timer;
  layer_timer.start();
  for (int layer_index = 0; layer_index < layer_count; ++layer_index) {
    layer_timer.restart();
    const double z = first_z + layer_index * params.layer_height;

    // Active triangle list: planes are visited in ascending Z, so triangles enter once their
    // minimum Z is reached and leave once their maximum Z is passed.
    while (next_triangle < order.size() &&
           tri_min_z[static_cast<size_t>(order[next_triangle])] <= z) {
      active.push_back(order[next_triangle]);
      ++next_triangle;
    }
    size_t write = 0;
    for (size_t read = 0; read < active.size(); ++read) {
      if (tri_max_z[static_cast<size_t>(active[read])] >= z) active[write++] = active[read];
    }
    active.resize(write);

    segments.clear();
    welder.clear();
    for (int index : active) {
      const Triangle &tri = work.triangles[static_cast<size_t>(index)];
      double d[3];
      int above = 0;
      int on_plane = 0;
      for (int i = 0; i < 3; ++i) {
        d[i] = tri.v[i].z - z;
        if (std::abs(d[i]) < plane_eps) {
          d[i] = plane_eps;  // consistent rule: a vertex on the plane counts as above it
          ++on_plane;
        }
        if (d[i] > 0.0) ++above;
      }
      if (on_plane == 3) {
        ++result.stats.coplanar_triangle_count;  // triangle lies in the plane, skipped (5-(f))
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
        crossing[found++] =
            QPointF(tri.v[i].x + (tri.v[j].x - tri.v[i].x) * t,
                    tri.v[i].y + (tri.v[j].y - tri.v[i].y) * t);
      }
      if (found != 2) continue;
      const int a = welder.insert(crossing[0].x(), crossing[0].y());
      const int b = welder.insert(crossing[1].x(), crossing[1].y());
      if (a != b) segments.emplace_back(a, b);
    }

    Layer layer;
    layer.index = layer_index;
    layer.z_geometry = z;
    layer.z_compensated = z;  // refractive index compensation not implemented yet (B-3)

    if (!segments.empty()) {
      const std::vector<QPointF> &points = welder.points();
      const std::vector<Chain> chains = chainSegments(
          segments, static_cast<int>(points.size()), &result.stats.non_manifold_junction_count);
      for (const Chain &chain : chains) {
        const size_t unique = chain.closed ? chain.point_ids.size() - 1 : chain.point_ids.size();
        if (unique < 2 || (chain.closed && unique < 3)) continue;
        Contour contour;
        contour.is_closed = chain.closed;
        contour.polygon.reserve(static_cast<int>(chain.point_ids.size()));
        for (int id : chain.point_ids) contour.polygon << points[static_cast<size_t>(id)];
        layer.contours.push_back(std::move(contour));
      }
      classifyContours(layer.contours, weld_tolerance);
    }

    layer.elapsed_us = layer_timer.nsecsElapsed() / 1000;
    for (const Contour &contour : layer.contours) {
      if (!contour.is_closed) ++result.stats.open_contour_count;
    }
    result.stats.contour_count += static_cast<int>(layer.contours.size());
    if (layer.contours.isEmpty()) ++result.stats.empty_layer_count;
    result.layers.push_back(std::move(layer));
  }

  result.stats.triangle_count = static_cast<int>(triangle_count);
  result.stats.layer_count = static_cast<int>(result.layers.size());
  result.stats.elapsed_ms = timer.elapsed();
  result.ok = true;
  return result;
}

QVector<SliceResult> sliceMeshes(const QVector<MeshInstance> &instances, const SliceParams &params) {
  QVector<SliceResult> results;
  results.reserve(instances.size());
  for (const MeshInstance &instance : instances) {
    results.push_back(sliceMesh(instance.mesh, instance.transform, params));
  }
  return results;
}

}  // namespace stl
