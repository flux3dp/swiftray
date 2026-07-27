#include "laser-path-filled.h"
#include <QLineF>
#include <QtMath>
#include <algorithm>
#include <cmath>
#include <functional>

namespace {
// Liang-Barsky: clip segment p0->p1 to rect r. Returns false if fully outside;
// otherwise p0/p1 are moved to the visible sub-segment (direction preserved).
bool clipSegmentToRect(QPointF& p0, QPointF& p1, const QRectF& r) {
  double x0 = p0.x(), y0 = p0.y();
  double dx = p1.x() - x0, dy = p1.y() - y0;
  double t0 = 0.0, t1 = 1.0;
  const double p[4] = {-dx, dx, -dy, dy};
  const double q[4] = {x0 - r.left(), r.right() - x0, y0 - r.top(),
                       r.bottom() - y0};
  for (int i = 0; i < 4; i++) {
    if (std::abs(p[i]) < 1e-12) {
      if (q[i] < 0) return false;
    } else {
      double t = q[i] / p[i];
      if (p[i] < 0) {
        if (t > t1) return false;
        if (t > t0) t0 = t;
      } else {
        if (t < t0) return false;
        if (t < t1) t1 = t;
      }
    }
  }
  p0 = QPointF(x0 + t0 * dx, y0 + t0 * dy);
  p1 = QPointF(x0 + t1 * dx, y0 + t1 * dy);
  return true;
}
}  // namespace

LaserPathFilledFactory::LaserPathFilledFactory(
    const FactoryKwargs& kwargs) noexcept
    : BaseFactory(kwargs),
      work_area_mm_(kwargs.work_area_mm),
      clip_rect_mm_(kwargs.clip_rect_mm) {
  qInfo() << "LaserPathFilledFactory created";
}

void LaserPathFilledFactory::add_filled_path(const QPainterPath& path_mm,
                                             bool is_even_odd) {
  FilledPath fill_path;
  fill_path.is_even_odd = is_even_odd;
  fill_path.polys = path_mm.toSubpathPolygons();
  filled_polygons_.append(fill_path);
}

QRectF LaserPathFilledFactory::compute_segments(
    QVector<FillSegment>& segments) {
  segments.clear();
  if (filled_polygons_.isEmpty()) {
    return QRectF();
  }
  struct Path {
    QLineF path;
    bool isClockwise;
  };
  struct PathGroup {
    QList<Path> paths;
    bool isEvenOdd;
  };
  struct Intersection {
    QPointF point;
    bool isClockwise;
  };

  // Rough dirty area = bounding rect of all filled polygons (work-area mm).
  QRectF bounds;
  for (const auto& paths : filled_polygons_) {
    for (const auto& poly : paths.polys) {
      if (poly.empty()) continue;
      bounds = bounds.united(poly.boundingRect());
    }
  }
  if (bounds.isEmpty()) {
    return QRectF();
  }

  double fill_interval = fill_params_.interval;  // mm
  if (fill_interval <= 0) fill_interval = 0.1;
  double fill_angle = fill_params_.angle;
  bool fill_bidirectional = fill_params_.bidirectional;
  int hatch_count = fill_params_.hatch_count;

  // Diagonal length (with margin) to ensure the scan lines cover the bounds.
  double diagonal = qSqrt(bounds.width() * bounds.width() +
                          bounds.height() * bounds.height()) *
                    1.1;
  if (diagonal == 0) {
    return bounds;
  }

  // Clip scan lines to the (work-area) layer clip, expressed in mm.
  PathUtils fill_path_utils;
  fill_path_utils.setClipRect(clip_rect_mm_.top,
                              work_area_mm_.width() - clip_rect_mm_.right,
                              work_area_mm_.height() - clip_rect_mm_.bottom,
                              clip_rect_mm_.left);

  int progress_batch = (hatch_count * diagonal * 1.5 / fill_interval) / 100 + 1;
  int cnt = -1;

  for (int hatch = 0; hatch < hatch_count; hatch++) {
    if (cancelled) return bounds;
    double angleRad = qDegreesToRadians(fill_angle);
    QPointF direction(qCos(angleRad), qSin(angleRad));
    QPointF perpendicular(-direction.y(), direction.x());
    QPointF center = bounds.center();
    QPointF start = center - (perpendicular * diagonal / 2);

    QList<PathGroup> all_paths;
    for (const auto& paths : filled_polygons_) {
      if (paths.polys.empty()) continue;
      PathGroup pathGroup;
      for (const auto& poly : paths.polys) {
        if (poly.empty()) continue;
        for (int i = 0; i < poly.size(); ++i) {
          QPointF curr = poly[i];
          QPointF next = poly[(i + 1) % poly.size()];  // wrap to first point
          double x1 = direction.x();
          double y1 = direction.y();
          double x2 = next.x() - curr.x();
          double y2 = next.y() - curr.y();
          double dir = x1 * y2 - x2 * y1;
          if (dir == 0) continue;
          Path pathObj;
          pathObj.path = QLineF(curr, next);
          pathObj.isClockwise = dir < 0;
          pathGroup.paths.append(pathObj);
        }
      }
      if (pathGroup.paths.size() == 0) continue;
      pathGroup.isEvenOdd = paths.is_even_odd;
      all_paths.append(pathGroup);
    }

    bool reverse = false;
    for (double offset_dist = -diagonal / 2; offset_dist <= diagonal;
         offset_dist += fill_interval) {
      if (cancelled) return bounds;
      if (++cnt % progress_batch == 0) {
        // Emit no progress delta, but pump events so cancellation is responsive.
        onProgressChanged(0, false);
      }
      QPointF lineStart =
          start + perpendicular * offset_dist - direction * diagonal / 2;
      QPointF lineEnd = lineStart + direction * diagonal;
      if (reverse) std::swap(lineStart, lineEnd);
      QLineF scanLine(lineStart, lineEnd);
      std::function<bool(const QPointF& a, const QPointF& b)> isCloser =
          [&lineStart](const QPointF& a, const QPointF& b) {
            return QLineF(lineStart, a).length() < QLineF(lineStart, b).length();
          };
      std::function<bool(const Intersection& a, const Intersection& b)>
          isCloserIntersection = [&lineStart](const Intersection& a,
                                              const Intersection& b) {
            return QLineF(lineStart, a.point).length() <
                   QLineF(lineStart, b.point).length();
          };

      QPointF innerStart(lineStart);
      QPointF innerEnd(lineEnd);
      if (fill_path_utils.clipWorkarea(&innerStart, &innerEnd) == -1) continue;

      QList<QList<QPointF>> all_intersections;
      QList<QPointF> merged_intersections;
      QList<int> indices(filled_polygons_.size());
      for (const auto& elem : all_paths) {
        QList<Intersection> intersections;
        QList<QPointF> intersection_points;
        for (const auto& path : elem.paths) {
          QPointF intersection;
          if (scanLine.intersects(path.path, &intersection) ==
              QLineF::BoundedIntersection) {
            Intersection intersectionObj;
            intersectionObj.point = intersection;
            intersectionObj.isClockwise = path.isClockwise;
            intersections.append(intersectionObj);
          }
        }
        if (intersections.size() == 0) continue;
        std::sort(intersections.begin(), intersections.end(),
                  isCloserIntersection);

        bool is_laser_on = false;
        int sum = 0;
        for (int i = 0; i < intersections.size(); i += 1) {
          Intersection intersection = intersections[i];
          if (intersection.isClockwise) sum += 1;
          else sum -= 1;
          bool should_laser_off = elem.isEvenOdd ? sum % 2 == 0 : sum == 0;
          if (is_laser_on == should_laser_off) {
            is_laser_on = !is_laser_on;
            if (isCloser(intersection.point, innerStart)) {
              intersection_points.append(innerStart);
            } else if (isCloser(innerEnd, intersection.point)) {
              intersection_points.append(innerEnd);
            } else {
              intersection_points.append(intersection.point);
            }
          }
        }
        all_intersections.append(intersection_points);
      }

      // Combine intersections of all polygons: find laser on/off pairs.
      merged_intersections.append(innerStart);
      merged_intersections.append(innerStart);
      QPointF lastOffPoint = innerStart;
      bool hasReachEnd = false;
      while (!hasReachEnd) {
        QPointF currentOnPoint = innerEnd;
        int currentIdx = -1;
        for (int i = 0; !hasReachEnd && i < all_intersections.size(); ++i) {
          for (int j = indices[i];
               !hasReachEnd && j < all_intersections[i].size(); j += 2) {
            QPointF seg_start = all_intersections[i][j];
            if (isCloser(seg_start, lastOffPoint) || seg_start == lastOffPoint) {
              QPointF seg_end = all_intersections[i][j + 1];
              if (isCloser(seg_end, lastOffPoint)) {
                indices[i] += 2;
                continue;
              }
              if (!isCloser(seg_end, innerEnd)) {
                seg_end = innerEnd;
                hasReachEnd = true;
              } else {
                indices[i] += 2;
              }
              lastOffPoint = seg_end;
              merged_intersections.last() = seg_end;
              continue;
            } else if (isCloser(seg_start, currentOnPoint)) {
              currentOnPoint = seg_start;
              currentIdx = i;
            }
            break;
          }
        }
        if (currentIdx == -1) break;

        merged_intersections.append(currentOnPoint);
        lastOffPoint = all_intersections[currentIdx][indices[currentIdx] + 1];
        if (!isCloser(lastOffPoint, innerEnd)) {
          lastOffPoint = innerEnd;
          hasReachEnd = true;
        }
        merged_intersections.append(lastOffPoint);
        indices[currentIdx] += 2;
      }

      // Collect the laser-on pairs as segments.
      for (int i = 0; i < merged_intersections.size() - 1; i += 2) {
        if (merged_intersections[i] == merged_intersections[i + 1]) {
          continue;  // skip zero-length segments
        }
        segments.append({merged_intersections[i], merged_intersections[i + 1]});
      }
      if (fill_bidirectional) reverse = !reverse;
    }
    fill_angle += 90;
  }
  return bounds;
}

QVector<LaserPathFilledFactory::FillBlock>
LaserPathFilledFactory::split_into_blocks(const QVector<FillSegment>& segments,
                                          const QRectF& bounds) const {
  QVector<FillBlock> blocks;
  if (block_regions_mm_.isEmpty()) {
    // No blocks: emit the whole fill as a single pass covering its bounds.
    FillBlock fb;
    fb.region = bounds;
    fb.segments = segments;
    blocks.append(fb);
    return blocks;
  }
  blocks.resize(block_regions_mm_.size());
  for (int i = 0; i < block_regions_mm_.size(); i++) {
    blocks[i].region = block_regions_mm_[i];
  }
  // Distribute each laser-on segment to every block it overlaps, clipped.
  for (const FillSegment& seg : segments) {
    QRectF seg_box = QRectF(seg.start, seg.end).normalized();
    for (int i = 0; i < block_regions_mm_.size(); i++) {
      if (!block_regions_mm_[i].intersects(seg_box)) continue;
      QPointF a = seg.start, b = seg.end;
      if (clipSegmentToRect(a, b, block_regions_mm_[i])) {
        blocks[i].segments.append({a, b});
      }
    }
  }
  return blocks;
}

void LaserPathFilledFactory::generate_task_code(float path_speed) {
  if (filled_polygons_.isEmpty()) {
    return;
  }
  QVector<FillSegment> segments;
  QRectF bounds = compute_segments(segments);
  if (cancelled || segments.isEmpty()) {
    return;
  }
  QVector<FillBlock> blocks = split_into_blocks(segments, bounds);

  bool block_mode = !block_regions_mm_.isEmpty();
  // Dev fluence handler: when enabled, each fill segment is emitted through the
  // ramp-compensating emitter (machine mm = work-area mm minus the layer
  // offset) instead of the plain travel-to-start / cut-to-end pair.
  bool use_fluence = fluence_.active();
  fluence_.set_run_params(fluence_power_pct_, path_speed);
  const QPointF off = offset;

  float current_pwm = -1;  // force first set_toolhead_pwm
  auto move_to = [&](const QPointF& p, bool laser_on) {
    float target_pwm = laser_on ? 100 : 0;
    if (current_pwm != target_pwm) {
      proc->set_toolhead_pwm(target_pwm);
      current_pwm = target_pwm;
    }
    NamedArgs args =
        NamedArgs().rx(p.x() - off.x()).ry(p.y() - off.y());
    if (laser_on) {
      args.rf(path_speed);
    } else {
      args.set_is_travel();
    }
    proc->moveto(args);
  };

  for (int i = 0; i < blocks.size(); i++) {
    if (cancelled) return;
    const FillBlock& fb = blocks[i];
    if (fb.segments.isEmpty()) {
      continue;
    }
    // One Promark task per block that has fill content: the callback moves the
    // head to the block and enters Promark mode; block_end exits. In non-block
    // mode there are no callbacks, so the single fill pass is emitted plainly
    // (matching the non-block path behaviour).
    if (block_mode && block_start_cb_) {
      block_start_cb_(i);
    }
    if (use_fluence) {
      // Cross-pass repeats the block's segments N times with alternating run
      // direction; the tube envelope is tracked across the passes (reset once
      // per block).
      fluence_.begin_block();
      int passes = fluence_.cross_pass();
      for (int pass = 0; pass < passes; pass++) {
        bool rev = (pass & 1) != 0;
        for (const FillSegment& seg : fb.segments) {
          QPointF a = seg.start - off;
          QPointF b = seg.end - off;
          if (rev) {
            fluence_.emit_run(b, a);
          } else {
            fluence_.emit_run(a, b);
          }
        }
      }
    } else {
      for (const FillSegment& seg : fb.segments) {
        move_to(seg.start, false);  // travel to start
        move_to(seg.end, true);     // cut to end
      }
      move_to(fb.segments.last().end, false);
    }
    if (block_mode && block_end_cb_) {
      block_end_cb_();
    }
    onProgressChanged(0, false);
  }
  // Laser off at the end of the fill.
  if (!use_fluence && current_pwm != 0) {
    proc->set_toolhead_pwm(0);
  }
}
