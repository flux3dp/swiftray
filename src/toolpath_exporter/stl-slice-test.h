#pragma once

#include <QJsonObject>
#include <QString>

/**
    \file stl-slice-test.h
    \brief Standalone exercise for the STL slicer (TODO.md, backend step 4).

    Two ways to run it, no command line arguments involved:
      - call runStlSliceTest() directly from any code path (a breakpoint in a debugger,
        a temporary call in main(), a unit test, ...)
      - over the daemon websocket: path "/ws/sr/system", action "sliceStlTest",
        params is an optional object with the same field names as StlSliceTestOptions

    Everything has a default, so `runStlSliceTest()` with no argument slices the hardcoded
    development model and runs every self check.
*/

struct StlSliceTestOptions {
  /** STL to slice. Defaults to the hardcoded development model. */
  QString file_path;
  double layer_height = 0.1;
  /** Dot mode point spacing; 0 skips the resampling pass. */
  double point_spacing = 0.0;
  /** Image output directory. Empty means <temp>/swiftray-slice-test. */
  QString output_dir;
  /** Write one image every N layers. */
  int image_stride = 1;
  /** Stop after N images; 0 means no limit. Dropped layers are reported, never silently skipped. */
  int max_images = 0;
  /** Slice through a non trivial 4x4 matrix instead of the identity (TODO.md B-4). */
  bool apply_demo_transform = false;
  bool write_images = true;
  /** Log every layer (TODO.md step 4-4); when false only every 100th layer is logged. */
  bool log_every_layer = true;
  bool run_self_checks = true;
  /** When false only the self checks run and no file is read. */
  bool slice_model = true;
};

struct StlSliceTestReport {
  bool ok = false;
  QString error;
  int checks_passed = 0;
  int checks_failed = 0;

  QString file_path;
  QString format;
  qint64 file_size = 0;
  int triangle_count = 0;
  int layer_count = 0;
  int contour_count = 0;
  int open_contour_count = 0;
  int empty_layer_count = 0;
  int coplanar_triangle_count = 0;
  int degenerate_triangle_count = 0;
  int non_manifold_junction_count = 0;
  int resampled_point_count = 0;
  qint64 read_ms = 0;
  qint64 slice_ms = 0;
  qint64 render_ms = 0;
  int images_written = 0;
  QString output_dir;

  QJsonObject toJson() const;
};

/**
 * Run the slicing exercise. Progress and per layer detail go to qInfo, the return value carries the
 * same numbers so a caller (or the websocket API) can assert on them.
 */
StlSliceTestReport runStlSliceTest(const StlSliceTestOptions &options = StlSliceTestOptions());

/** Build the options from a websocket params object; every field is optional. */
StlSliceTestOptions stlSliceTestOptionsFromJson(const QJsonObject &json);
