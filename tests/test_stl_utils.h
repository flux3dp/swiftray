#pragma once

#include <gtest/gtest.h>

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
