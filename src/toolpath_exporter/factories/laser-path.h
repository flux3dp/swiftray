#pragma once

#include "base-factory.h"
#include "toolpath_exporter/toolpath-utils.h"

class LaserPathFactory : public BaseFactory {
 private:
  PathUtils path_utils;
  double loop_compensation_mm;
  QVector<QPolygonF> polygons;  // px when adding, mm after preprocess
  QVector<QPolygonF> polygons_backup; // px, a copy of the original polygons for block processing
  bool preprocessed = false;
  bool use_ga_ = true;
  float current_pwm = 0;
  float speed;

  void preprocess();
  void walk_path(QPointF point, bool should_emit);

  bool has_block_clip_ = false;
  QRectF block_clip_;  // px

 public:
  LaserPathFactory(const FactoryKwargs& kwargs) noexcept;

  void set_loop_compensation(double val);
  void set_use_ga(bool use_ga) { use_ga_ = use_ga; }
  void add_path(const QPainterPath& path);
  int get_size();
  void generate_task_code(float path_speed);
  // Restrict subsequent generate_task_code() calls to `block` (work-area mm).
  // Paths are still sorted/pre-processed once; only the polyline pieces inside
  // the block are emitted. Used for block-split laser tasks.
  void set_block_clip(const QRectF& block);
  void clear_block_clip() { has_block_clip_ = false; }
};
