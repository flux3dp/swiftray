#include "stl-slice-test.h"

#include <QColor>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QImage>
#include <QPainter>
#include <QPen>
#include <QStringList>
#include <QtMath>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <initializer_list>

#include "stl-utils.h"

namespace {

/** Development model used when no file path is given (TODO.md step 4-1). */
const char *kDefaultStlPath = "/Users/software/Downloads/Mcqueen Car.STL";

class Checker {
 public:
  void check(bool ok, const QString &what) {
    if (ok) {
      ++passed_;
      qInfo().noquote() << "  [PASS]" << what;
    } else {
      ++failed_;
      qWarning().noquote() << "  [FAIL]" << what;
    }
  }

  int passed() const { return passed_; }

  int failed() const { return failed_; }

 private:
  int passed_ = 0;
  int failed_ = 0;
};

bool nearly(double a, double b, double tolerance) { return std::abs(a - b) <= tolerance; }

void appendUint32LE(QByteArray &out, quint32 value) {
  const char bytes[4] = {static_cast<char>(value & 0xff), static_cast<char>((value >> 8) & 0xff),
                         static_cast<char>((value >> 16) & 0xff),
                         static_cast<char>((value >> 24) & 0xff)};
  out.append(bytes, 4);
}

void appendFloatLE(QByteArray &out, float value) {
  quint32 bits = 0;
  std::memcpy(&bits, &value, sizeof(bits));
  appendUint32LE(out, bits);
}

/** Serialise a mesh as binary STL, with a caller supplied 80 byte header. */
QByteArray buildBinaryStl(const stl::Mesh &mesh, const QByteArray &header_text) {
  QByteArray header(80, ' ');
  const int copy = std::min<int>(80, header_text.size());
  for (int i = 0; i < copy; ++i) header[i] = header_text[i];

  QByteArray out = header;
  appendUint32LE(out, static_cast<quint32>(mesh.triangles.size()));
  for (const stl::Triangle &tri : mesh.triangles) {
    for (int i = 0; i < 3; ++i) appendFloatLE(out, 0.0f);  // normal, deliberately zero
    for (int v = 0; v < 3; ++v) {
      appendFloatLE(out, static_cast<float>(tri.v[v].x));
      appendFloatLE(out, static_cast<float>(tri.v[v].y));
      appendFloatLE(out, static_cast<float>(tri.v[v].z));
    }
    out.append(2, '\0');  // attribute byte count
  }
  return out;
}

QByteArray buildAsciiStl(const stl::Mesh &mesh) {
  QByteArray out = "solid test\n";
  for (const stl::Triangle &tri : mesh.triangles) {
    out += "  facet normal 0 0 0\n    outer loop\n";
    for (int v = 0; v < 3; ++v) {
      out += QStringLiteral("      vertex %1 %2 %3\n")
                 .arg(tri.v[v].x, 0, 'g', 9)
                 .arg(tri.v[v].y, 0, 'g', 9)
                 .arg(tri.v[v].z, 0, 'g', 9)
                 .toLatin1();
    }
    out += "    endloop\n  endfacet\n";
  }
  out += "endsolid test\n";
  return out;
}

stl::Triangle makeTriangle(const stl::Vec3 &a, const stl::Vec3 &b, const stl::Vec3 &c) {
  stl::Triangle tri;
  tri.v[0] = a;
  tri.v[1] = b;
  tri.v[2] = c;
  return tri;
}

/** Axis aligned box, 12 triangles. */
stl::Mesh makeBox(double x0, double y0, double z0, double x1, double y1, double z1) {
  const stl::Vec3 p000{x0, y0, z0};
  const stl::Vec3 p100{x1, y0, z0};
  const stl::Vec3 p110{x1, y1, z0};
  const stl::Vec3 p010{x0, y1, z0};
  const stl::Vec3 p001{x0, y0, z1};
  const stl::Vec3 p101{x1, y0, z1};
  const stl::Vec3 p111{x1, y1, z1};
  const stl::Vec3 p011{x0, y1, z1};

  stl::Mesh mesh;
  mesh.triangles = {
      makeTriangle(p000, p110, p100), makeTriangle(p000, p010, p110),  // bottom
      makeTriangle(p001, p101, p111), makeTriangle(p001, p111, p011),  // top
      makeTriangle(p000, p100, p101), makeTriangle(p000, p101, p001),  // y0
      makeTriangle(p010, p111, p110), makeTriangle(p010, p011, p111),  // y1
      makeTriangle(p000, p001, p011), makeTriangle(p000, p011, p010),  // x0
      makeTriangle(p100, p110, p111), makeTriangle(p100, p111, p101),  // x1
  };
  return mesh;
}

/** Watertight tube: outer wall, inner wall and two annular caps. */
stl::Mesh makeHollowCylinder(double cx, double cy, double z0, double z1, double outer_radius,
                             double inner_radius, int segments) {
  stl::Mesh mesh;
  auto point = [&](double radius, int i, double z) {
    const double angle = 2.0 * M_PI * i / segments;
    return stl::Vec3{cx + radius * std::cos(angle), cy + radius * std::sin(angle), z};
  };
  for (int i = 0; i < segments; ++i) {
    const int j = (i + 1) % segments;
    const stl::Vec3 ob0 = point(outer_radius, i, z0);
    const stl::Vec3 ob1 = point(outer_radius, j, z0);
    const stl::Vec3 ot0 = point(outer_radius, i, z1);
    const stl::Vec3 ot1 = point(outer_radius, j, z1);
    const stl::Vec3 ib0 = point(inner_radius, i, z0);
    const stl::Vec3 ib1 = point(inner_radius, j, z0);
    const stl::Vec3 it0 = point(inner_radius, i, z1);
    const stl::Vec3 it1 = point(inner_radius, j, z1);
    mesh.triangles.push_back(makeTriangle(ob0, ob1, ot1));
    mesh.triangles.push_back(makeTriangle(ob0, ot1, ot0));
    mesh.triangles.push_back(makeTriangle(ib0, it1, ib1));
    mesh.triangles.push_back(makeTriangle(ib0, it0, it1));
    mesh.triangles.push_back(makeTriangle(ot0, ot1, it1));
    mesh.triangles.push_back(makeTriangle(ot0, it1, it0));
    mesh.triangles.push_back(makeTriangle(ob0, ib1, ob1));
    mesh.triangles.push_back(makeTriangle(ob0, ib0, ib1));
  }
  return mesh;
}

QPolygonF makePolyline(std::initializer_list<QPointF> points) {
  QPolygonF poly;
  for (const QPointF &p : points) poly << p;
  return poly;
}

QString describeContour(const stl::Contour &contour) {
  return QStringLiteral("%1 pts=%2 %3 parent=%4 depth=%5")
      .arg(contour.is_closed ? QStringLiteral("closed") : QStringLiteral("OPEN"))
      .arg(contour.polygon.size())
      .arg(contour.orientation > 0   ? QStringLiteral("ccw")
           : contour.orientation < 0 ? QStringLiteral("cw")
                                     : QStringLiteral("--"))
      .arg(contour.parent_index)
      .arg(contour.depth);
}

/**
 * One PNG per layer. Plain black on white would not show whether chaining and the outer / hole
 * classification are correct, so: outer black, hole red, open polyline blue, colour fades along the
 * point order and the start point gets a green marker.
 */
bool renderLayer(const stl::Layer &layer, const QRectF &bounds, const QString &file_path,
                 int max_pixels) {
  const double span = std::max(bounds.width(), bounds.height());
  const double scale = (span > 0.0) ? (max_pixels - 40) / span : 1.0;
  const int width = std::max(64, static_cast<int>(bounds.width() * scale) + 40);
  const int height = std::max(64, static_cast<int>(bounds.height() * scale) + 40);

  QImage image(width, height, QImage::Format_RGB32);
  image.fill(Qt::white);
  QPainter painter(&image);
  painter.setRenderHint(QPainter::Antialiasing, true);

  // Y stays pointing down to match the SVG / canvas convention (TODO.md B-4).
  auto map = [&](const QPointF &p) {
    return QPointF(20.0 + (p.x() - bounds.left()) * scale, 20.0 + (p.y() - bounds.top()) * scale);
  };

  for (const stl::Contour &contour : layer.contours) {
    QColor base = Qt::black;
    if (!contour.is_closed) {
      base = QColor(30, 90, 220);
    } else if (contour.depth % 2 == 1) {
      base = QColor(220, 40, 40);
    }
    const QPolygonF &poly = contour.polygon;
    for (int i = 0; i + 1 < poly.size(); ++i) {
      const double t = (poly.size() > 2) ? static_cast<double>(i) / (poly.size() - 2) : 0.0;
      QColor color(base.red() + static_cast<int>((200 - base.red()) * t),
                   base.green() + static_cast<int>((200 - base.green()) * t),
                   base.blue() + static_cast<int>((200 - base.blue()) * t));
      painter.setPen(QPen(color, 1.2));
      painter.drawLine(map(poly[i]), map(poly[i + 1]));
    }
    if (!poly.isEmpty()) {
      painter.setPen(Qt::NoPen);
      painter.setBrush(QColor(20, 160, 60));
      painter.drawEllipse(map(poly.first()), 2.5, 2.5);
      painter.setBrush(Qt::NoBrush);
    }
  }

  painter.setPen(Qt::darkGray);
  painter.drawText(8, 14,
                   QStringLiteral("layer %1  z=%2  contours=%3")
                       .arg(layer.index)
                       .arg(layer.z_geometry, 0, 'f', 3)
                       .arg(layer.contours.size()));
  painter.end();
  return image.save(file_path);
}

/** Covers the special cases listed in TODO.md section 5. */
void runSelfChecks(int *passed, int *failed) {
  Checker checker;
  qInfo().noquote() << "== self checks ==";

  // 5-(a) binary STL whose header starts with "solid " must not be taken for ASCII.
  const stl::Mesh box = makeBox(0, 0, 0, 10, 10, 10);
  const QByteArray binary_with_solid_header = buildBinaryStl(box, "solid Mcqueen_20160506");
  checker.check(stl::detectFormat(binary_with_solid_header) == stl::Format::Binary,
                "5-(a) binary STL with a \"solid\" header is detected as binary");
  {
    stl::Mesh parsed;
    QString error;
    const bool ok = stl::readMesh(binary_with_solid_header, &parsed, &error);
    checker.check(ok && parsed.triangles.size() == 12,
                  QStringLiteral("5-(a) it parses into 12 triangles (%1)").arg(error));
  }

  // 5-(b) ASCII STL.
  {
    stl::Mesh parsed;
    QString error;
    const QByteArray ascii = buildAsciiStl(box);
    const bool ok = stl::readMesh(ascii, &parsed, &error);
    checker.check(stl::detectFormat(ascii) == stl::Format::Ascii && ok &&
                      parsed.triangles.size() == 12,
                  QStringLiteral("5-(b) ASCII STL parses into 12 triangles (%1)").arg(error));
  }

  // 5-(c) truncated binary must be rejected, not read out of bounds.
  {
    stl::Mesh parsed;
    QString error;
    QByteArray truncated = binary_with_solid_header;
    truncated.chop(120);
    checker.check(!stl::readMesh(truncated, &parsed, &error) && !error.isEmpty(),
                  "5-(c) truncated binary STL is rejected with an error");
  }

  // 5-(d) empty / zero triangle input.
  {
    stl::Mesh parsed;
    QString error;
    checker.check(!stl::readMesh(QByteArray(), &parsed, &error), "5-(d) empty file is rejected");
    const QByteArray zero_faces = buildBinaryStl(stl::Mesh(), "solid empty");
    checker.check(!stl::readMesh(zero_faces, &parsed, &error),
                  "5-(d) binary STL declaring 0 triangles is rejected");
  }

  stl::SliceParams params;
  params.layer_height = 1.0;

  // Basic cube: one closed CCW contour per layer with the known area.
  {
    const stl::SliceResult result = stl::sliceMesh(box, QMatrix4x4(), params);
    bool shape_ok = result.ok && result.layers.size() == 10;
    bool area_ok = shape_ok;
    for (const stl::Layer &layer : result.layers) {
      if (layer.contours.size() != 1) {
        shape_ok = false;
        break;
      }
      const stl::Contour &c = layer.contours.front();
      // The cross section of a cube is a square, but each side face is two triangles, so the
      // contour legitimately has more than 4 vertices (collinear points are not merged).
      if (!c.is_closed || c.orientation != 1 || c.parent_index != -1 ||
          !nearly(std::abs(stl::signedArea(c.polygon)), 100.0, 1e-6) ||
          c.polygon.first() != c.polygon.last()) {
        area_ok = false;
        break;
      }
    }
    checker.check(shape_ok, "cube: 10 layers with exactly one contour each");
    checker.check(area_ok, "cube: every contour is closed, CCW, area 100, first point == last");
  }

  // 5-(e) + 5-(f): two stacked boxes share a horizontal face exactly on a slicing plane, so that
  // plane hits vertices and is coplanar with two faces.
  {
    stl::Mesh stacked = makeBox(0, 0, 0, 10, 10, 5.5);
    const stl::Mesh upper = makeBox(0, 0, 5.5, 10, 10, 11);
    stacked.triangles.insert(stacked.triangles.end(), upper.triangles.begin(),
                             upper.triangles.end());
    const stl::SliceResult result = stl::sliceMesh(stacked, QMatrix4x4(), params);
    const stl::Layer *plane_layer = nullptr;
    for (const stl::Layer &layer : result.layers) {
      if (nearly(layer.z_geometry, 5.5, 1e-9)) plane_layer = &layer;
    }
    checker.check(plane_layer != nullptr && plane_layer->contours.size() == 1 &&
                      plane_layer->contours.front().is_closed &&
                      nearly(std::abs(stl::signedArea(plane_layer->contours.front().polygon)), 100.0,
                             1e-6),
                  "5-(e) a plane through shared vertices still yields exactly one closed contour");
    checker.check(result.stats.coplanar_triangle_count > 0,
                  QStringLiteral("5-(f) coplanar faces are detected and skipped (%1 triangles)")
                      .arg(result.stats.coplanar_triangle_count));
  }

  // 5-(g) degenerate triangles are dropped.
  {
    stl::Mesh with_degenerate = box;
    const stl::Vec3 a{1, 1, 1};
    with_degenerate.triangles.push_back(makeTriangle(a, a, a));
    with_degenerate.triangles.push_back(makeTriangle(stl::Vec3{0, 0, 0}, stl::Vec3{1, 1, 1},
                                                     stl::Vec3{2, 2, 2}));
    const stl::SliceResult result = stl::sliceMesh(with_degenerate, QMatrix4x4(), params);
    checker.check(result.ok && result.stats.degenerate_triangle_count == 2,
                  "5-(g) zero area triangles are counted and skipped");
  }

  // 5-(h) broken mesh: removing side triangles must produce open polylines, not a crash.
  {
    stl::Mesh broken = box;
    broken.triangles.erase(broken.triangles.begin() + 8, broken.triangles.begin() + 10);
    const stl::SliceResult result = stl::sliceMesh(broken, QMatrix4x4(), params);
    bool has_open = false;
    for (const stl::Layer &layer : result.layers) {
      for (const stl::Contour &c : layer.contours) {
        if (!c.is_closed) has_open = true;
      }
    }
    checker.check(result.ok && has_open && result.stats.open_contour_count > 0,
                  "5-(h) non watertight mesh yields open polylines flagged is_closed=false");
  }

  // B-7 nesting: a tube must give an outer contour plus a hole, with opposite winding.
  {
    const stl::Mesh tube = makeHollowCylinder(0, 0, 0, 10, 20, 10, 64);
    const stl::SliceResult result = stl::sliceMesh(tube, QMatrix4x4(), params);
    bool ok = result.ok && !result.layers.isEmpty();
    for (const stl::Layer &layer : result.layers) {
      if (layer.contours.size() != 2) {
        ok = false;
        break;
      }
      const stl::Contour &a = layer.contours[0];
      const stl::Contour &b = layer.contours[1];
      const stl::Contour &outer = (a.depth == 0) ? a : b;
      const stl::Contour &hole = (a.depth == 0) ? b : a;
      const int hole_index = (a.depth == 0) ? 1 : 0;
      const int outer_index = 1 - hole_index;
      if (outer.depth != 0 || outer.parent_index != -1 || outer.orientation != 1) ok = false;
      if (hole.depth != 1 || hole.parent_index != outer_index || hole.orientation != -1) ok = false;
      if (!nearly(std::abs(stl::signedArea(outer.polygon)), M_PI * 400.0, 5.0)) ok = false;
      if (!nearly(std::abs(stl::signedArea(hole.polygon)), M_PI * 100.0, 5.0)) ok = false;
    }
    checker.check(ok, "B-7 hollow cylinder: outer CCW at depth 0, hole CW at depth 1 pointing at it");
  }

  // 5-(i) two separate shells in one layer stay independent (no false nesting).
  {
    stl::Mesh two_shells = makeBox(0, 0, 0, 10, 10, 10);
    const stl::Mesh second = makeBox(40, 0, 0, 50, 10, 10);
    two_shells.triangles.insert(two_shells.triangles.end(), second.triangles.begin(),
                                second.triangles.end());
    const stl::SliceResult result = stl::sliceMesh(two_shells, QMatrix4x4(), params);
    bool ok = result.ok && !result.layers.isEmpty();
    for (const stl::Layer &layer : result.layers) {
      if (layer.contours.size() != 2) {
        ok = false;
        break;
      }
      for (const stl::Contour &c : layer.contours) {
        if (c.depth != 0 || c.parent_index != -1 || c.orientation != 1) ok = false;
      }
    }
    checker.check(ok, "5-(i) two disjoint shells give two independent outer contours");
  }

  // 5-(j) a mesh far from the origin, with negative coordinates.
  {
    const stl::Mesh offset_box = makeBox(-30.25, -12.5, -7.75, -20.25, -2.5, 2.25);
    const stl::SliceResult result = stl::sliceMesh(offset_box, QMatrix4x4(), params);
    checker.check(result.ok && result.layers.size() == 10 &&
                      nearly(result.layers.first().z_geometry, -7.25, 1e-9),
                  "5-(j) slicing starts from the real bbox, not from z = 0");
  }

  // 5-(m) empty layers are kept so the layer index keeps mapping to a Z.
  {
    stl::Mesh gapped = makeBox(0, 0, 0, 10, 10, 2);
    const stl::Mesh upper = makeBox(0, 0, 8, 10, 10, 10);
    gapped.triangles.insert(gapped.triangles.end(), upper.triangles.begin(), upper.triangles.end());
    const stl::SliceResult result = stl::sliceMesh(gapped, QMatrix4x4(), params);
    bool indices_ok = result.ok && result.layers.size() == 10;
    for (int i = 0; i < result.layers.size(); ++i) {
      if (result.layers[i].index != i) indices_ok = false;
    }
    checker.check(indices_ok && result.stats.empty_layer_count > 0,
                  QStringLiteral("5-(m) the gap produces %1 empty layers without renumbering")
                      .arg(result.stats.empty_layer_count));
  }

  // 5-(n) invalid or oversized layer height.
  {
    stl::SliceParams bad = params;
    bad.layer_height = 0.0;
    checker.check(!stl::sliceMesh(box, QMatrix4x4(), bad).ok, "5-(n) layer height 0 is rejected");
    bad.layer_height = -1.0;
    checker.check(!stl::sliceMesh(box, QMatrix4x4(), bad).ok,
                  "5-(n) negative layer height is rejected");
    stl::SliceParams thick = params;
    thick.layer_height = 100.0;
    const stl::SliceResult result = stl::sliceMesh(box, QMatrix4x4(), thick);
    checker.check(result.ok && result.layers.size() == 1 && result.layers.first().contours.size() == 1,
                  "5-(n) layer height above the model height yields one layer that is not empty");
  }

  // 5-(o) the first plane sits half a layer above the bottom.
  {
    const stl::SliceResult result = stl::sliceMesh(box, QMatrix4x4(), params);
    checker.check(result.ok && nearly(result.layers.first().z_geometry, 0.5, 1e-9) &&
                      result.layers.first().index == 0,
                  "5-(o) the first plane is offset by half a layer, avoiding the bbox face");
  }

  // Layers are ordered deep -> shallow, i.e. ascending Z (B-5).
  {
    const stl::SliceResult result = stl::sliceMesh(box, QMatrix4x4(), params);
    bool ascending = result.ok;
    for (int i = 1; i < result.layers.size(); ++i) {
      if (result.layers[i].z_geometry <= result.layers[i - 1].z_geometry) ascending = false;
    }
    checker.check(ascending, "B-5 layers are ordered bottom to top (deep to shallow)");
  }

  // B-4 the 4x4 matrix path: scale, rotate, translate.
  {
    const stl::Mesh flat = makeBox(0, 0, 0, 10, 20, 4);
    QMatrix4x4 matrix;
    matrix.translate(5.0f, 5.0f, 5.0f);
    matrix.rotate(90.0f, 1.0f, 0.0f, 0.0f);
    matrix.scale(2.0f);
    const stl::SliceResult result = stl::sliceMesh(flat, matrix, params);
    // 10x20x4 scaled by 2 is 20x40x8; rotating 90 degrees about X turns the 40 into the height.
    bool ok = result.ok && result.layers.size() == 40;
    for (const stl::Layer &layer : result.layers) {
      if (layer.contours.size() != 1 ||
          !nearly(std::abs(stl::signedArea(layer.contours.front().polygon)), 160.0, 1e-6)) {
        ok = false;
        break;
      }
    }
    checker.check(ok, "B-4 a non trivial 4x4 matrix (scale + rotate + translate) is applied");
  }

  // 5-(p) resampling rules: round the gap count, then spread the gaps evenly.
  {
    // Straight polylines only: on a contour with corners two consecutive samples can straddle a
    // corner, and then their straight line distance is legitimately shorter than the arc step.
    auto gapsAreEven = [](const QPolygonF &poly, double expected_step) {
      if (poly.size() < 2) return false;
      for (int i = 0; i + 1 < poly.size(); ++i) {
        const QPointF d = poly[i + 1] - poly[i];
        if (!nearly(std::sqrt(d.x() * d.x() + d.y() * d.y()), expected_step, 1e-6)) return false;
      }
      return true;
    };

    const QPolygonF open_line = makePolyline({QPointF(0, 0), QPointF(10, 0)});
    const QPolygonF open_sampled = stl::resamplePolygon(open_line, 1.0, false);
    checker.check(open_sampled.size() == 11 && gapsAreEven(open_sampled, 1.0),
                  "5-(p) open 10mm path at 1mm gives 11 evenly spaced points");

    const QPolygonF long_line = makePolyline({QPointF(0, 0), QPointF(10.5, 0)});
    const QPolygonF long_sampled = stl::resamplePolygon(long_line, 1.0, false);
    // round(10.5 / 1) = 11 gaps -> 12 points, every gap 10.5 / 11
    checker.check(long_sampled.size() == 12 && nearly(long_sampled.last().x(), 10.5, 1e-9) &&
                      gapsAreEven(long_sampled, 10.5 / 11.0),
                  QStringLiteral("5-(p) open 10.5mm at 1mm rounds to 11 even gaps, nothing is "
                                 "discarded (got %1 points)")
                      .arg(long_sampled.size()));

    const QPolygonF square = makePolyline({QPointF(0, 0), QPointF(2.5, 0), QPointF(2.5, 2.5),
                                           QPointF(0, 2.5), QPointF(0, 0)});
    const QPolygonF closed_sampled = stl::resamplePolygon(square, 1.0, true);
    checker.check(closed_sampled.size() == 10 && closed_sampled.first() == QPointF(0, 0) &&
                      closed_sampled.last() != closed_sampled.first(),
                  QStringLiteral("5-(p) closed 10mm contour at 1mm gives 10 points, start not "
                                 "repeated (got %1)")
                      .arg(closed_sampled.size()));

    // A closed contour whose length is not a multiple of the spacing must not end up with a short
    // seam gap: round(10.5) = 11 gaps of 10.5 / 11, including the one that closes the loop.
    const QPolygonF odd_square = makePolyline({QPointF(0, 0), QPointF(2.625, 0),
                                               QPointF(2.625, 2.625), QPointF(0, 2.625),
                                               QPointF(0, 0)});
    const QPolygonF odd_sampled = stl::resamplePolygon(odd_square, 1.0, true);
    // The last sample sits one step before the start, measured along the left edge, so here the
    // straight line distance back to the start is the arc step.
    const QPointF seam = odd_sampled.isEmpty() ? QPointF() : odd_sampled.last() - odd_sampled.first();
    checker.check(odd_sampled.size() == 11 &&
                      nearly(std::sqrt(seam.x() * seam.x() + seam.y() * seam.y()), 10.5 / 11.0, 1e-6),
                  QStringLiteral("5-(p) closed 10.5mm contour has an even seam, no over exposed "
                                 "point (got %1 points)")
                      .arg(odd_sampled.size()));

    const QPolygonF tiny = makePolyline({QPointF(0, 0), QPointF(0.4, 0)});
    const QPolygonF tiny_sampled = stl::resamplePolygon(tiny, 1.0, false);
    checker.check(tiny_sampled.size() == 2,
                  "5-(p) a contour shorter than the spacing keeps both of its endpoints");
    const QPolygonF tiny_closed = makePolyline({QPointF(0, 0), QPointF(0.2, 0), QPointF(0, 0)});
    checker.check(stl::resamplePolygon(tiny_closed, 1.0, true).size() == 1,
                  "5-(p) a closed contour shorter than the spacing yields its start point");
  }

  // Slicing at an arbitrary Z, which is what a shared multi object Z ladder needs.
  {
    stl::SliceParams slicer_params;
    slicer_params.layer_height = 1.0;
    stl::Slicer slicer;
    QString error;
    const bool prepared = slicer.prepare(box, QMatrix4x4(), slicer_params, &error);
    checker.check(prepared && nearly(slicer.minZ(), 0.0, 1e-9) &&
                      nearly(slicer.maxZ(), 10.0, 1e-9) && slicer.planes().size() == 10,
                  QStringLiteral("Slicer::prepare reports the Z range and 10 planes (%1)").arg(error));
    const stl::Layer at_3_7 = slicer.sliceAt(3.7);
    checker.check(at_3_7.contours.size() == 1 &&
                      nearly(std::abs(stl::signedArea(at_3_7.contours.front().polygon)), 100.0, 1e-6),
                  "Slicer::sliceAt slices at an arbitrary Z, not only at the layer positions");
    // Walking backwards has to restart the sweep instead of returning nothing.
    const stl::Layer at_1_2 = slicer.sliceAt(1.2);
    checker.check(at_1_2.contours.size() == 1 &&
                      nearly(std::abs(stl::signedArea(at_1_2.contours.front().polygon)), 100.0, 1e-6),
                  "Slicer::sliceAt going back down restarts the sweep and still finds the contour");
    const stl::Layer outside = slicer.sliceAt(20.0);
    checker.check(outside.contours.isEmpty(), "Slicer::sliceAt outside the mesh returns nothing");
  }

  qInfo().noquote() << QStringLiteral("== self checks: %1 passed, %2 failed ==")
                           .arg(checker.passed())
                           .arg(checker.failed());
  if (passed != nullptr) *passed = checker.passed();
  if (failed != nullptr) *failed = checker.failed();
}

}  // namespace

QJsonObject StlSliceTestReport::toJson() const {
  QJsonObject json;
  json["ok"] = ok;
  if (!error.isEmpty()) json["error"] = error;
  json["checksPassed"] = checks_passed;
  json["checksFailed"] = checks_failed;
  json["filePath"] = file_path;
  json["format"] = format;
  json["fileSize"] = static_cast<double>(file_size);
  json["triangleCount"] = triangle_count;
  json["layerCount"] = layer_count;
  json["contourCount"] = contour_count;
  json["openContourCount"] = open_contour_count;
  json["emptyLayerCount"] = empty_layer_count;
  json["coplanarTriangleCount"] = coplanar_triangle_count;
  json["degenerateTriangleCount"] = degenerate_triangle_count;
  json["nonManifoldJunctionCount"] = non_manifold_junction_count;
  json["resampledPointCount"] = resampled_point_count;
  json["readMs"] = static_cast<double>(read_ms);
  json["sliceMs"] = static_cast<double>(slice_ms);
  json["renderMs"] = static_cast<double>(render_ms);
  json["imagesWritten"] = images_written;
  json["outputDir"] = output_dir;
  return json;
}

StlSliceTestOptions stlSliceTestOptionsFromJson(const QJsonObject &json) {
  StlSliceTestOptions options;
  if (json.contains("filePath")) options.file_path = json["filePath"].toString();
  if (json.contains("layerHeight")) options.layer_height = json["layerHeight"].toDouble();
  if (json.contains("pointSpacing")) options.point_spacing = json["pointSpacing"].toDouble();
  if (json.contains("outputDir")) options.output_dir = json["outputDir"].toString();
  if (json.contains("imageStride")) options.image_stride = json["imageStride"].toInt();
  if (json.contains("maxImages")) options.max_images = json["maxImages"].toInt();
  if (json.contains("applyDemoTransform")) options.apply_demo_transform = json["applyDemoTransform"].toBool();
  if (json.contains("writeImages")) options.write_images = json["writeImages"].toBool();
  if (json.contains("logEveryLayer")) options.log_every_layer = json["logEveryLayer"].toBool();
  if (json.contains("runSelfChecks")) options.run_self_checks = json["runSelfChecks"].toBool();
  if (json.contains("sliceModel")) options.slice_model = json["sliceModel"].toBool();
  return options;
}

StlSliceTestReport runStlSliceTest(const StlSliceTestOptions &options) {
  StlSliceTestReport report;
  report.file_path =
      options.file_path.isEmpty() ? QString::fromUtf8(kDefaultStlPath) : options.file_path;
  report.output_dir = options.output_dir.isEmpty()
                          ? "/Users/software/Downloads/slice-test"
                          : options.output_dir;

  if (options.run_self_checks) {
    runSelfChecks(&report.checks_passed, &report.checks_failed);
  }
  if (!options.slice_model) {
    report.ok = report.checks_failed == 0;
    return report;
  }

  qInfo().noquote() << "";
  qInfo().noquote() << "== step 1: read" << report.file_path << "==";
  QElapsedTimer timer;
  timer.start();

  QFile file(report.file_path);
  if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
    report.error = QStringLiteral("cannot open STL file: %1").arg(report.file_path);
    qWarning().noquote() << report.error;
    return report;
  }
  const QByteArray data = file.readAll();
  file.close();
  report.file_size = data.size();

  const stl::Format format = stl::detectFormat(data);
  report.format = (format == stl::Format::Binary)  ? QStringLiteral("binary")
                  : (format == stl::Format::Ascii) ? QStringLiteral("ascii")
                                                   : QStringLiteral("unknown");
  stl::Mesh mesh;
  QString error;
  if (!stl::readMesh(data, &mesh, &error)) {
    report.error = error;
    qWarning().noquote() << "failed to read STL:" << error;
    return report;
  }
  report.read_ms = timer.elapsed();
  report.triangle_count = static_cast<int>(mesh.triangles.size());
  // The header text is logged on purpose: the development model starts with "solid ..." although it
  // is binary, which is exactly the trap described in TODO.md 5-(a).
  qInfo().noquote() << QStringLiteral("format=%1  header starts with \"%2\"  size=%3 bytes  "
                                      "triangles=%4  read in %5 ms")
                           .arg(report.format)
                           .arg(QString::fromLatin1(data.left(16).trimmed()))
                           .arg(report.file_size)
                           .arg(report.triangle_count)
                           .arg(report.read_ms);

  stl::Vec3 lo;
  stl::Vec3 hi;
  mesh.boundingBox(&lo, &hi);
  qInfo().noquote() << QStringLiteral("bbox min=(%1, %2, %3)  max=(%4, %5, %6)  size=(%7, %8, %9)")
                           .arg(lo.x, 0, 'f', 3).arg(lo.y, 0, 'f', 3).arg(lo.z, 0, 'f', 3)
                           .arg(hi.x, 0, 'f', 3).arg(hi.y, 0, 'f', 3).arg(hi.z, 0, 'f', 3)
                           .arg(hi.x - lo.x, 0, 'f', 3)
                           .arg(hi.y - lo.y, 0, 'f', 3)
                           .arg(hi.z - lo.z, 0, 'f', 3);

  qInfo().noquote() << "== step 2: transform ==";
  QMatrix4x4 matrix;
  if (options.apply_demo_transform) {
    // Deliberately non trivial: scale, two rotations and a translation, so the model goes through
    // the same 4x4 path the frontend matrix will take (TODO.md B-4).
    matrix.translate(15.0f, -8.0f, 3.0f);
    matrix.rotate(20.0f, 1.0f, 0.0f, 0.0f);
    matrix.rotate(35.0f, 0.0f, 0.0f, 1.0f);
    matrix.scale(1.5f);
  }
  qInfo().noquote() << (options.apply_demo_transform
                            ? "applying a scale + rotate + translate matrix"
                            : "identity (set apply_demo_transform to exercise the 4x4 path)");

  qInfo().noquote() << QStringLiteral("== step 3: slice at %1 per layer ==").arg(options.layer_height);
  stl::SliceParams params;
  params.layer_height = options.layer_height;
  const stl::SliceResult result = stl::sliceMesh(mesh, matrix, params);
  if (!result.ok) {
    report.error = result.error;
    qWarning().noquote() << "slicing failed:" << result.error;
    return report;
  }

  qInfo().noquote() << "== step 4: layers ==";
  for (const stl::Layer &layer : result.layers) {
    if (!options.log_every_layer && layer.index % 100 != 0) continue;
    QStringList details;
    for (int i = 0; i < layer.contours.size() && i < 8; ++i) {
      details << QStringLiteral("c%1[%2]").arg(i).arg(describeContour(layer.contours[i]));
    }
    if (layer.contours.size() > 8) {
      details << QStringLiteral("... +%1 more").arg(layer.contours.size() - 8);
    }
    qInfo().noquote() << QStringLiteral("layer %1/%2  z=%3  contours=%4  %5us  %6")
                             .arg(layer.index + 1, 5)
                             .arg(result.layers.size())
                             .arg(layer.z_geometry, 10, 'f', 3)
                             .arg(layer.contours.size(), 3)
                             .arg(layer.elapsed_us, 7)
                             .arg(details.join(QStringLiteral("  ")));
  }

  if (options.point_spacing > 0.0) {
    for (const stl::Layer &layer : result.layers) {
      for (const stl::Contour &contour : layer.contours) {
        report.resampled_point_count += static_cast<int>(
            stl::resamplePolygon(contour.polygon, options.point_spacing, contour.is_closed).size());
      }
    }
    qInfo().noquote() << QStringLiteral("dot mode: %1 points at %2 spacing")
                             .arg(report.resampled_point_count)
                             .arg(options.point_spacing);
  }

  if (options.write_images) {
    qInfo().noquote() << "== step 5: images ->" << report.output_dir << "==";
    const int stride = std::max(1, options.image_stride);
    if (!QDir().mkpath(report.output_dir)) {
      qWarning().noquote() << "cannot create output directory" << report.output_dir;
    } else {
      // One shared frame for every layer, otherwise the contours would jump around between images.
      QRectF bounds;
      for (const stl::Layer &layer : result.layers) {
        for (const stl::Contour &contour : layer.contours) {
          bounds = bounds.isNull() ? contour.polygon.boundingRect()
                                   : bounds.united(contour.polygon.boundingRect());
        }
      }
      if (bounds.isNull()) {
        qWarning().noquote() << "no contour to draw";
      } else {
        QElapsedTimer render_timer;
        render_timer.start();
        int skipped_by_limit = 0;
        for (const stl::Layer &layer : result.layers) {
          if (layer.index % stride != 0) continue;
          if (options.max_images > 0 && report.images_written >= options.max_images) {
            ++skipped_by_limit;
            continue;
          }
          const QString name = QStringLiteral("layer_%1_z%2.png")
                                   .arg(layer.index, 5, 10, QChar('0'))
                                   .arg(layer.z_geometry, 0, 'f', 3);
          if (renderLayer(layer, bounds, report.output_dir + QStringLiteral("/") + name, 800)) {
            ++report.images_written;
          }
        }
        report.render_ms = render_timer.elapsed();
        qInfo().noquote() << QStringLiteral("wrote %1 images in %2 ms")
                                 .arg(report.images_written)
                                 .arg(report.render_ms);
        // Never let a cap look like full coverage.
        if (skipped_by_limit > 0) {
          qWarning().noquote() << QStringLiteral("max_images reached: %1 further layers were NOT "
                                                 "drawn")
                                      .arg(skipped_by_limit);
        }
        if (stride > 1) {
          qWarning().noquote() << QStringLiteral("stride %1: only every %1th layer was drawn")
                                      .arg(stride);
        }
      }
    }
  }

  report.layer_count = result.stats.layer_count;
  report.contour_count = result.stats.contour_count;
  report.open_contour_count = result.stats.open_contour_count;
  report.empty_layer_count = result.stats.empty_layer_count;
  report.coplanar_triangle_count = result.stats.coplanar_triangle_count;
  report.degenerate_triangle_count = result.stats.degenerate_triangle_count;
  report.non_manifold_junction_count = result.stats.non_manifold_junction_count;
  report.slice_ms = result.stats.elapsed_ms;
  report.ok = report.checks_failed == 0;

  qInfo().noquote() << "== summary ==";
  qInfo().noquote() << QStringLiteral("triangles           : %1").arg(report.triangle_count);
  qInfo().noquote() << QStringLiteral("layers              : %1").arg(report.layer_count);
  qInfo().noquote() << QStringLiteral("contours            : %1").arg(report.contour_count);
  qInfo().noquote() << QStringLiteral("open contours       : %1").arg(report.open_contour_count);
  qInfo().noquote() << QStringLiteral("empty layers        : %1").arg(report.empty_layer_count);
  qInfo().noquote() << QStringLiteral("coplanar triangles  : %1").arg(report.coplanar_triangle_count);
  qInfo().noquote() << QStringLiteral("degenerate triangles: %1").arg(report.degenerate_triangle_count);
  qInfo().noquote() << QStringLiteral("non manifold points : %1").arg(report.non_manifold_junction_count);
  qInfo().noquote() << QStringLiteral("slicing time        : %1 ms").arg(report.slice_ms);
  qInfo().noquote() << QStringLiteral("self checks         : %1 passed, %2 failed")
                           .arg(report.checks_passed)
                           .arg(report.checks_failed);
  return report;
}

