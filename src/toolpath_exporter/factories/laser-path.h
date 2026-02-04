#pragma once

#include "base-factory.h"
#include "toolpath_exporter/toolpath-utils.h"

class LaserPathFactory : public BaseFactory {
 private:
  PathUtils path_utils;
  double loop_compensation_mm;
  QVector<QPolygonF> polygons;  // px when adding, mm after preprocess
  bool preprocessed = false;
  float current_pwm = 0;
  float speed;

  void preprocess();
  void walk_path(QPointF point, bool should_emit);

 public:
  LaserPathFactory(const FactoryKwargs& kwargs) noexcept;

  void set_loop_compensation(double val);
  void add_path(const QPainterPath& path);
  int get_size();
  void generate_task_code(float path_speed);
};
