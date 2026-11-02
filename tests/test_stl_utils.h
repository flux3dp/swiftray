#pragma once

#include <gtest/gtest.h>

#include <cmath>

#include <toolpath_exporter/refraction-compensator.h>
#include <toolpath_exporter/stl-utils.h>

namespace {

stl::Mesh makeTestTetrahedron() {
  stl::Mesh mesh;
  mesh.triangles = {
      {{{0, 0, 0}, {10, 0, 0}, {0, 10, 0}}},
      {{{0, 0, 0}, {0, 10, 0}, {0, 0, 10}}},
      {{{0, 0, 0}, {0, 0, 10}, {10, 0, 0}}},
      {{{10, 0, 0}, {0, 0, 10}, {0, 10, 0}}},
  };
  return mesh;
}

}  // namespace

TEST(RefractionCompensatorTest, AlwaysUsesBasicAxialMapping) {
  RefractionCompensator compensator;
  compensator.setParams(RefractionParams{1.5, 30.0});

  EXPECT_DOUBLE_EQ(compensator.machineZ(30.0), 30.0);
  EXPECT_DOUBLE_EQ(compensator.machineZ(0.0), 10.0);
  EXPECT_DOUBLE_EQ(compensator.machineZ(15.0), 20.0);
}

TEST(RefractionCompensatorTest, IndexOneIsIdentity) {
  RefractionCompensator compensator;
  compensator.setParams(RefractionParams{1.0, 30.0});
  EXPECT_DOUBLE_EQ(compensator.machineZ(7.25), 7.25);
}

TEST(StlUtilsTest, SurfaceSamplingCanBeCancelled) {
  int callbacks = 0;
  const stl::PointCloudResult result = stl::sampleSurfaceBlueNoise(
      makeTestTetrahedron(), QMatrix4x4(), stl::BlueNoiseParams{},
      [&](double) { return ++callbacks < 2; });

  EXPECT_FALSE(result.ok);
  EXPECT_EQ(result.error, QStringLiteral("cancelled"));
  EXPECT_GE(callbacks, 2);
}

TEST(StlUtilsTest, SlicerPreparationCanBeCancelled) {
  stl::Slicer slicer;
  stl::SliceParams params;
  QString error;
  int callbacks = 0;

  EXPECT_FALSE(slicer.prepare(makeTestTetrahedron(), QMatrix4x4(), params, &error,
                              [&](double) { return ++callbacks < 2; }));
  EXPECT_EQ(error, QStringLiteral("cancelled"));
  EXPECT_FALSE(slicer.isReady());
}

namespace {

void addQuad(stl::Mesh *mesh, stl::Vec3 a, stl::Vec3 b, stl::Vec3 c, stl::Vec3 d) {
  mesh->triangles.push_back({{a, b, c}});
  mesh->triangles.push_back({{a, c, d}});
}

/** Axis-aligned box, wound outward. */
void addBox(stl::Mesh *mesh, double x0, double y0, double z0, double x1, double y1, double z1) {
  addQuad(mesh, {x0, y0, z1}, {x1, y0, z1}, {x1, y1, z1}, {x0, y1, z1});
  addQuad(mesh, {x0, y0, z0}, {x0, y1, z0}, {x1, y1, z0}, {x1, y0, z0});
  addQuad(mesh, {x0, y0, z0}, {x1, y0, z0}, {x1, y0, z1}, {x0, y0, z1});
  addQuad(mesh, {x1, y0, z0}, {x1, y1, z0}, {x1, y1, z1}, {x1, y0, z1});
  addQuad(mesh, {x1, y1, z0}, {x0, y1, z0}, {x0, y1, z1}, {x1, y1, z1});
  addQuad(mesh, {x0, y1, z0}, {x0, y0, z0}, {x0, y0, z1}, {x0, y1, z1});
}

/** Even-odd area of a cap: holes come back wound the other way. */
double capArea(const stl::CapFace &cap) {
  double area = 0;
  for (const QPolygonF &polygon : cap.polygons) {
    for (int i = 0; i < polygon.size(); ++i) {
      const QPointF &a = polygon[i];
      const QPointF &b = polygon[(i + 1) % polygon.size()];
      area += a.x() * b.y() - b.x() * a.y();
    }
  }
  return std::abs(area) / 2;
}

QVector<stl::CapFace> capFacesOf(const stl::Mesh &mesh, double inset,
                                 const QMatrix4x4 &transform = QMatrix4x4(),
                                 double layer_height = 0.4) {
  stl::Slicer slicer;
  stl::SliceParams params;
  params.layer_height = layer_height;
  EXPECT_TRUE(slicer.prepare(mesh, transform, params));
  return slicer.capFaces(inset);
}

}  // namespace

TEST(StlUtilsTest, CapFacesAreTheTopAndBottomShrunkByTheInset) {
  stl::Mesh cube;
  addBox(&cube, 0, 0, 0, 10, 10, 5);

  const QVector<stl::CapFace> caps = capFacesOf(cube, 0.5);
  ASSERT_EQ(caps.size(), 2);
  EXPECT_NEAR(caps[0].z, 0, 1e-9);
  EXPECT_NEAR(caps[1].z, 5, 1e-9);
  EXPECT_NEAR(capArea(caps[0]), 81, 1e-3);
  EXPECT_NEAR(capArea(caps[1]), 81, 1e-3);
  // a face narrower than twice the inset disappears
  EXPECT_TRUE(capFacesOf(cube, 6).isEmpty());
}

TEST(StlUtilsTest, CapFacesLeaveHolesEmpty) {
  stl::Mesh frame;  // 10 x 10 with a 4 x 4 hole through it
  addBox(&frame, 0, 0, 0, 10, 3, 5);
  addBox(&frame, 0, 7, 0, 10, 10, 5);
  addBox(&frame, 0, 3, 0, 3, 7, 5);
  addBox(&frame, 7, 3, 0, 10, 7, 5);

  const QVector<stl::CapFace> caps = capFacesOf(frame, 0);
  ASSERT_EQ(caps.size(), 2);
  EXPECT_EQ(caps[1].polygons.size(), 2);
  EXPECT_NEAR(capArea(caps[1]), 84, 1e-3);
}

TEST(StlUtilsTest, CapFacesSkipWhereTwoShellsTouch) {
  stl::Mesh step;  // a 5 x 10 block standing on a 10 x 10 one, two shells
  addBox(&step, 0, 0, 0, 10, 10, 2);
  addBox(&step, 0, 0, 2, 5, 10, 4);

  const QVector<stl::CapFace> caps = capFacesOf(step, 0);
  ASSERT_EQ(caps.size(), 3);
  EXPECT_NEAR(capArea(caps[0]), 100, 1e-3);
  // only the uncovered half of the step is a face
  EXPECT_NEAR(caps[1].z, 2, 1e-9);
  EXPECT_NEAR(capArea(caps[1]), 50, 1e-3);
  EXPECT_NEAR(capArea(caps[2]), 50, 1e-3);
}

TEST(StlUtilsTest, CapFacesKeepBothFacesOfAPlateThinnerThanHalfALayer) {
  stl::Mesh plate;
  addBox(&plate, 0, 0, 0, 10, 10, 0.4);

  // top and bottom are one fill, not an up face cancelling a down face
  const QVector<stl::CapFace> caps = capFacesOf(plate, 0, QMatrix4x4(), 1.0);
  ASSERT_EQ(caps.size(), 1);
  EXPECT_NEAR(caps[0].z, 0.2, 1e-9);
  EXPECT_NEAR(capArea(caps[0]), 100, 1e-3);

  stl::Mesh foil;
  addBox(&foil, 0, 0, 0, 10, 10, 0.01);
  ASSERT_EQ(capFacesOf(foil, 0, QMatrix4x4(), 0.04).size(), 1);
}

TEST(StlUtilsTest, CapFacesSkipWhereLeaningShellsTouch) {
  stl::Mesh step;
  addBox(&step, 0, 0, 0, 10, 10, 2);
  addBox(&step, 0, 0, 2, 5, 10, 4);

  // leaning, the touching faces span some Z but still lie on one another
  QMatrix4x4 leaning;
  leaning.rotate(0.5, 1, 0, 0);
  const QVector<stl::CapFace> caps = capFacesOf(step, 0, leaning);
  ASSERT_EQ(caps.size(), 3);
  EXPECT_NEAR(capArea(caps[1]), 50, 0.1);
}

TEST(StlUtilsTest, CapFacesCanBeCancelled) {
  stl::Mesh cube;
  addBox(&cube, 0, 0, 0, 10, 10, 5);
  stl::Slicer slicer;
  ASSERT_TRUE(slicer.prepare(cube, QMatrix4x4(), stl::SliceParams{}));

  int callbacks = 0;
  EXPECT_TRUE(slicer.capFaces(0, [&](double) { return ++callbacks < 2; }).isEmpty());
  EXPECT_EQ(callbacks, 2);
}

TEST(StlUtilsTest, CapFacesFollowTheTransform) {
  stl::Mesh cube;
  addBox(&cube, 0, 0, 0, 10, 10, 5);

  QMatrix4x4 leaning;
  leaning.rotate(0.5, 1, 0, 0);
  // a face leaning less than kCapMaxTiltDeg is still one face
  EXPECT_EQ(capFacesOf(cube, 0, leaning).size(), 2);

  QMatrix4x4 tilted;
  tilted.scale(-1, 1, 1);
  tilted.rotate(30, 1, 0, 0);
  EXPECT_TRUE(capFacesOf(cube, 0, tilted).isEmpty());
}

TEST(StlUtilsTest, RemoveClosePointsKeepsTheFirstOfEachCluster) {
  std::vector<stl::SurfacePoint> points = {
      {{0, 0, 0}, 0},    {{0.3, 0, 0}, 0}, {{1, 0, 0}, 0},
      {{1, 0.4, 0}, 0},  {{1, 0, 0.6}, 0}, {{5, 5, 5}, 0},
  };

  EXPECT_EQ(stl::removeClosePoints(&points, 0.5), 2u);
  ASSERT_EQ(points.size(), 4u);
  EXPECT_NEAR(points[0].position.x, 0, 1e-12);
  EXPECT_NEAR(points[1].position.x, 1, 1e-12);
  // 0.6 away in Z alone is far enough
  EXPECT_NEAR(points[2].position.z, 0.6, 1e-12);
  EXPECT_NEAR(points[3].position.x, 5, 1e-12);
  EXPECT_EQ(stl::removeClosePoints(&points, 0), 0u);
}

TEST(StlUtilsTest, RemoveClosePointsReportsProgressAndCanBeCancelled) {
  std::vector<stl::SurfacePoint> points;
  for (int i = 0; i < 10000; ++i) points.push_back({{static_cast<double>(i), 0, 0}, 0});

  std::vector<double> reported;
  std::vector<stl::SurfacePoint> all = points;
  EXPECT_EQ(stl::removeClosePoints(&all, 0.5, [&](double value) {
              reported.push_back(value);
              return true;
            }),
            0u);
  EXPECT_EQ(all.size(), 10000u);
  ASSERT_GE(reported.size(), 3u);
  EXPECT_DOUBLE_EQ(reported.back(), 1.0);

  int callbacks = 0;
  EXPECT_EQ(stl::removeClosePoints(&points, 0.5, [&](double) { return ++callbacks < 2; }), 0u);
  EXPECT_EQ(callbacks, 2);
}
