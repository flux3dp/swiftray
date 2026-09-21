#include "laser-path-filled.h"

#include <QtMath>
#include <algorithm>

LaserPathFilledFactory::LaserPathFilledFactory(
    const FactoryKwargs& kwargs) noexcept
    : BaseFactory(kwargs) {
  qInfo() << "LaserPathFilledFactory created";
  clip_mm_ = QRectF(
      kwargs.clip_rect_mm.left, kwargs.clip_rect_mm.top,
      qMax(kwargs.work_area_mm.width() - kwargs.clip_rect_mm.left -
               kwargs.clip_rect_mm.right,
           0.0),
      qMax(kwargs.work_area_mm.height() - kwargs.clip_rect_mm.top -
               kwargs.clip_rect_mm.bottom,
           0.0));
}

void LaserPathFilledFactory::add_filled_path(const QPainterPath& path_mm,
                                             bool is_even_odd) {
  FilledPath filled;
  filled.polys = path_mm.toSubpathPolygons();
  filled.is_even_odd = is_even_odd;
  if (!filled.polys.isEmpty()) {
    filled_.append(filled);
  }
}

QRectF LaserPathFilledFactory::bounds() const {
  QRectF box;
  for (const FilledPath& filled : filled_) {
    for (const QPolygonF& poly : filled.polys) {
      box = box.united(poly.boundingRect());
    }
  }
  return box;
}

QRectF LaserPathFilledFactory::get_bounds_mm() const {
  const QRectF box = bounds().intersected(clip_mm_);
  return box.isEmpty() ? box : box.translated(-offset);
}

QList<LaserPathFilledFactory::EdgeGroup> LaserPathFilledFactory::buildEdges(
    const QPointF& direction) const {
  QList<EdgeGroup> groups;
  for (const FilledPath& filled : filled_) {
    EdgeGroup group;
    group.is_even_odd = filled.is_even_odd;
    for (const QPolygonF& poly : filled.polys) {
      for (int i = 0; i < poly.size(); i++) {
        const QPointF current = poly[i];
        // wrap, so an outline left open still closes
        const QPointF next = poly[(i + 1) % poly.size()];
        // Which side of the scan direction this edge runs: the sign of the
        // cross product is what tells an entry from an exit further down.
        const double cross = direction.x() * (next.y() - current.y()) -
                             (next.x() - current.x()) * direction.y();
        if (cross == 0) {
          continue;  // parallel to the scan, never a crossing
        }
        Edge edge;
        edge.line = QLineF(current, next);
        edge.is_clockwise = cross < 0;
        group.edges.append(edge);
      }
    }
    if (!group.edges.isEmpty()) {
      groups.append(group);
    }
  }
  return groups;
}

QList<QPointF> LaserPathFilledFactory::spansOn(
    const QLineF& scan_line,
    const QPointF& inner_start,
    const QPointF& inner_end,
    const QList<EdgeGroup>& groups) const {
  struct Crossing {
    QPointF point;
    bool is_clockwise;
  };
  auto closer = [&inner_start](const QPointF& a, const QPointF& b) {
    return QLineF(inner_start, a).length() < QLineF(inner_start, b).length();
  };

  // Per outline, the stretches of this scan line that fall inside it.
  QList<QList<QPointF>> per_group;
  for (const EdgeGroup& group : groups) {
    QList<Crossing> crossings;
    for (const Edge& edge : group.edges) {
      QPointF point;
      if (scan_line.intersects(edge.line, &point) == QLineF::BoundedIntersection) {
        crossings.append({point, edge.is_clockwise});
      }
    }
    if (crossings.isEmpty()) {
      continue;
    }
    std::sort(crossings.begin(), crossings.end(),
              [&closer](const Crossing& a, const Crossing& b) {
                return closer(a.point, b.point);
              });
    // Walk the crossings keeping a winding count; even-odd and non-zero differ
    // only in when that count means "outside".
    QList<QPointF> points;
    bool laser_on = false;
    int winding = 0;
    for (const Crossing& crossing : crossings) {
      winding += crossing.is_clockwise ? 1 : -1;
      const bool should_be_off =
          group.is_even_odd ? winding % 2 == 0 : winding == 0;
      if (laser_on != should_be_off) {
        continue;  // no change of state
      }
      laser_on = !laser_on;
      if (closer(crossing.point, inner_start)) {
        points.append(inner_start);
      } else if (closer(inner_end, crossing.point)) {
        points.append(inner_end);
      } else {
        points.append(crossing.point);
      }
    }
    if (!points.isEmpty()) {
      per_group.append(points);
    }
  }

  // Merge the outlines: where two of them overlap along this line the beam
  // should pass once, so the spans are walked in order and joined.
  QList<QPointF> merged;
  QList<int> indices(per_group.size(), 0);
  merged.append(inner_start);
  merged.append(inner_start);
  QPointF last_off = inner_start;
  bool reached_end = false;
  while (!reached_end) {
    QPointF next_on = inner_end;
    int next_group = -1;
    for (int i = 0; !reached_end && i < per_group.size(); i++) {
      for (int j = indices[i]; !reached_end && j + 1 < per_group[i].size();
           j += 2) {
        const QPointF start = per_group[i][j];
        if (closer(start, last_off) || start == last_off) {
          QPointF end = per_group[i][j + 1];
          if (closer(end, last_off)) {
            indices[i] += 2;  // wholly behind us already
            continue;
          }
          if (!closer(end, inner_end)) {
            end = inner_end;
            reached_end = true;
          } else {
            indices[i] += 2;
          }
          last_off = end;  // this span overlaps the open one; extend it
          merged.last() = end;
          continue;
        } else if (closer(start, next_on)) {
          next_on = start;
          next_group = i;
        }
        break;
      }
    }
    if (next_group == -1) {
      break;
    }
    merged.append(next_on);
    last_off = per_group[next_group][indices[next_group] + 1];
    if (!closer(last_off, inner_end)) {
      last_off = inner_end;
      reached_end = true;
    }
    merged.append(last_off);
    indices[next_group] += 2;
  }
  return merged;
}

void LaserPathFilledFactory::emitSegment(const QPointF& from,
                                         const QPointF& to,
                                         float speed) {
  // The power has to be set before the move, not after: a fill is a run of
  // two-point segments, so a move that arrives before the laser is on would
  // travel where it should be cutting.
  if (current_pwm_ != 0) {
    current_pwm_ = 0;
    proc->set_toolhead_pwm(0);
  }
  proc->moveto(NamedArgs()
                   .rx(from.x() - offset.x())
                   .ry(from.y() - offset.y())
                   .set_is_travel());
  current_pwm_ = 100;
  proc->set_toolhead_pwm(100);
  proc->moveto(NamedArgs()
                   .rf(speed)
                   .rx(to.x() - offset.x())
                   .ry(to.y() - offset.y()));
}

void LaserPathFilledFactory::generate_task_code(float speed) {
  if (filled_.isEmpty()) {
    return;
  }
  const QRectF box = bounds().intersected(clip_mm_);
  if (box.isEmpty()) {
    return;
  }
  double interval = params_.interval;
  if (interval <= 0) {
    interval = 0.1;
  }
  const double diagonal =
      qSqrt(box.width() * box.width() + box.height() * box.height());
  if (diagonal == 0) {
    return;
  }
  current_pwm_ = 0;

  double angle = params_.angle;
  const int hatches = qMax(params_.hatch_count, 1);
  for (int hatch = 0; hatch < hatches; hatch++) {
    if (cancelled) {
      return;
    }
    const double radians = qDegreesToRadians(angle);
    const QPointF direction(qCos(radians), qSin(radians));
    const QPointF perpendicular(-direction.y(), direction.x());
    const QPointF start = box.center() - perpendicular * diagonal / 2;
    const QList<EdgeGroup> groups = buildEdges(direction);
    bool reverse = false;

    for (double at = -diagonal / 2; at <= diagonal; at += interval) {
      if (cancelled) {
        return;
      }
      QPointF line_start =
          start + perpendicular * at - direction * diagonal / 2;
      QPointF line_end = line_start + direction * diagonal;
      if (reverse) {
        std::swap(line_start, line_end);
      }
      const QLineF scan_line(line_start, line_end);

      // Keep the run inside the fill's own bounds; the beam has no business
      // past them even where an outline would say otherwise.
      QPointF inner_start = line_start;
      QPointF inner_end = line_end;
      QPointF clipped;
      const QLineF edges[4] = {
          QLineF(box.topLeft(), box.topRight()),
          QLineF(box.topRight(), box.bottomRight()),
          QLineF(box.bottomRight(), box.bottomLeft()),
          QLineF(box.bottomLeft(), box.topLeft())};
      QList<QPointF> hits;
      for (const QLineF& edge : edges) {
        if (scan_line.intersects(edge, &clipped) == QLineF::BoundedIntersection) {
          hits.append(clipped);
        }
      }
      if (hits.size() >= 2) {
        std::sort(hits.begin(), hits.end(),
                  [&line_start](const QPointF& a, const QPointF& b) {
                    return QLineF(line_start, a).length() <
                           QLineF(line_start, b).length();
                  });
        inner_start = hits.first();
        inner_end = hits.last();
      } else if (!box.contains(line_start) || !box.contains(line_end)) {
        continue;  // misses the fill entirely
      }

      const QList<QPointF> merged =
          spansOn(scan_line, inner_start, inner_end, groups);
      for (int i = 0; i + 1 < merged.size(); i += 2) {
        if (merged[i] == merged[i + 1]) {
          continue;  // zero length
        }
        emitSegment(merged[i], merged[i + 1], speed);
      }
      if (params_.bidirectional) {
        reverse = !reverse;
      }
    }
    angle += 90;  // the cross of a cross-hatch
  }

  if (current_pwm_ != 0) {
    current_pwm_ = 0;
    proc->set_toolhead_pwm(0);
  }
}
