#include "laser-path.h"

LaserPathFactory::LaserPathFactory(const FactoryKwargs& kwargs) noexcept
    : BaseFactory(kwargs) {
  qInfo() << "LaserPathFactory created";
  path_utils.setClipRect(clip_rect.top(), clip_rect.right(), clip_rect.bottom(), clip_rect.left());
}

void LaserPathFactory::set_loop_compensation(double val) {
  loop_compensation_mm = val;
  path_utils.setLoopCompensation(loop_compensation_mm * pixel_per_mm);
}

void LaserPathFactory::add_path(const QPainterPath& path) {
  polygons.append(path.toSubpathPolygons());
}

int LaserPathFactory::get_size() {
  return polygons.size();
}

void LaserPathFactory::preprocess() {
  if (preprocessed) {
    return;
  }
  path_utils.sortAndPreprocessPolygons(polygons, use_ga_);
  auto convert = [this](QPointF pt) -> QPointF {
    return pt / pixel_per_mm - offset;
  };
  for (auto& poly : polygons) {
    std::transform(poly.begin(), poly.end(), poly.begin(), convert);
  }
  preprocessed = true;
}

void LaserPathFactory::generate_task_code(float path_speed) {
  speed = path_speed;
  current_pwm = 0;
  preprocess();
  for (auto& poly : polygons) {
    if (poly.isEmpty()) {
      continue;
    }
    walk_path(poly.first(), false);
    for (QPointF& point : poly) {
      walk_path(point, true);
    }
    walk_path(poly.last(), false);
  }
}

void LaserPathFactory::walk_path(QPointF point, bool should_emit) {
  if (should_emit) {
    proc->moveto(speed, point.x(), point.y());
  } else {
    proc->moveto(NAN, point.x(), point.y(), NAN, NAN, NAN, NAN, true);
  }
  float target_power = should_emit ? 100 : 0;
  if (current_pwm != target_power) {
    current_pwm = target_power;
    proc->set_toolhead_pwm(target_power);
  }
}
