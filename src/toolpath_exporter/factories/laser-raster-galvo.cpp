#include "laser-raster-galvo.h"

#include <QDebug>
#include <QtGlobal>
#include <cmath>

#include "toolpath_exporter/generators/galvo-list.h"

namespace {

// The ordered screen the ownership targets are drawn from, 20 phases over a
// 5x4 cell. Ordered rather than random so neighbouring sites fall to different
// owners in a fixed interleave, with no clumping of its own.
constexpr int kScreenWidth = 5;
constexpr int kScreenHeight = 4;
constexpr int kScreenPhases = 20;
constexpr int kScreen[kScreenHeight][kScreenWidth] = {
    {0, 12, 3, 15, 6},
    {10, 5, 18, 9, 13},
    {4, 16, 1, 14, 7},
    {19, 8, 11, 2, 17},
};
constexpr double kEpsilon = 1e-9;
// Ownership phases for runs, and how many of them a cell's height is divided
// into so the interlocking shifts along as the scan walks down.
constexpr int kRunPhases = 100;
constexpr int kRunRowsPerStep = 64;

/**
 * How deep into a seam's band a coordinate sits, on one axis. Zero through the
 * cell's core, reaching 1 at the far edge of the band -- and complementary
 * between the two blocks sharing that seam, so their two progresses always sum
 * to one.
 */
double axisProgress(double coordinate,
                    double nominal_min,
                    double nominal_max,
                    double overlap,
                    bool has_lower,
                    bool has_upper) {
  const double inset = overlap / 2;
  if (has_lower && coordinate < nominal_min + inset) {
    return qMin(1.0, (nominal_min + inset - coordinate) / overlap);
  }
  if (has_upper && coordinate > nominal_max - inset) {
    return qMin(1.0, (coordinate - (nominal_max - inset)) / overlap);
  }
  return 0;
}

}  // namespace

double LaserRasterGalvoFactory::profileDensity(double progress) const {
  if (progress <= kEpsilon) {
    return 1;  // the core keeps every dot
  }
  switch (blend_.profile) {
    case BlendProfile::Simple:
      return 0.5;
    case BlendProfile::SuperGranular: {
      const int band = qMin(9, int(qMax(0.0, progress - kEpsilon) * 10));
      return band < 5 ? 0.9 - band * 0.1 : 0.5 - (band - 5) * 0.1;
    }
    case BlendProfile::Granular:
    default:
      if (progress <= 0.25 + kEpsilon) return 0.75;
      if (progress <= 0.75 + kEpsilon) return 0.5;
      return 0.25;
  }
}

QRectF LaserRasterGalvoFactory::cellAt(int col, int row) const {
  const double sx = blend_.step.width();
  const double sy = blend_.step.height();
  return QRectF(col * sx - sx / 2, row * sy - sy / 2, sx, sy);
}

QRectF LaserRasterGalvoFactory::bandAt(int col, int row) const {
  const double half = blend_.overlap / 2;
  return cellAt(col, row).adjusted(col > blend_.min_col ? -half : 0,
                                   row > blend_.min_row ? -half : 0,
                                   col < blend_.max_col ? half : 0,
                                   row < blend_.max_row ? half : 0);
}

QRectF LaserRasterGalvoFactory::blend_band() const {
  return bandAt(blend_.col, blend_.row);
}

double LaserRasterGalvoFactory::rawWeight(int col,
                                          int row,
                                          double x,
                                          double y) const {
  const QRectF cell = cellAt(col, row);
  // The two axes multiply rather than taking the deeper of them. That is what
  // makes a corner work: four cells each claim a quarter and the quarters come
  // to one, where the deeper-of-the-two reading would have them all claim a
  // half and burn the corner twice over.
  return profileDensity(axisProgress(x, cell.left(), cell.right(),
                                     blend_.overlap, col > blend_.min_col,
                                     col < blend_.max_col)) *
         profileDensity(axisProgress(y, cell.top(), cell.bottom(),
                                     blend_.overlap, row > blend_.min_row,
                                     row < blend_.max_row));
}

double LaserRasterGalvoFactory::siteTarget(int column, int row) const {
  const int sx = ((column % kScreenWidth) + kScreenWidth) % kScreenWidth;
  const int sy = ((row % kScreenHeight) + kScreenHeight) % kScreenHeight;
  return (kScreen[sy][sx] + 0.5) / kScreenPhases;
}

double LaserRasterGalvoFactory::runTarget(double x, double y) const {
  // Coarse cells along the scan line, offset row by row, so the seam
  // interlocks like brickwork instead of running straight. The multipliers are
  // the reference implementation's: coprime with the phase count, so
  // neighbouring cells and rows land far apart in the ordering.
  const double length = blend_.segment_length > 0 ? blend_.segment_length : 2;
  const long long cell = static_cast<long long>(std::floor(x / length));
  const long long row =
      static_cast<long long>(std::floor(y / qMax(blend_.step.height(), 1e-6) *
                                        kRunRowsPerStep));
  long long phase = (cell * 61 + row * 37) % kRunPhases;
  if (phase < 0) {
    phase += kRunPhases;
  }
  return (phase + 0.5) / kRunPhases;
}

bool LaserRasterGalvoFactory::ownsSite(double x,
                                       double y,
                                       double target) const {
  struct Candidate {
    int col;
    int row;
    double weight;
  };
  const double sx = blend_.step.width();
  const double sy = blend_.step.height();
  const int near_col = int(std::floor(x / sx + 0.5));
  const int near_row = int(std::floor(y / sy + 0.5));
  QVector<Candidate> candidates;
  double total = 0;
  // Every cell whose band covers this site is a candidate. Walked row then
  // column, so the order is the same for whoever asks.
  for (int r = qMax(blend_.min_row, near_row - 1);
       r <= qMin(blend_.max_row, near_row + 1); r++) {
    for (int c = qMax(blend_.min_col, near_col - 1);
         c <= qMin(blend_.max_col, near_col + 1); c++) {
      const QRectF band = bandAt(c, r);
      if (x < band.left() - kEpsilon || x > band.right() + kEpsilon ||
          y < band.top() - kEpsilon || y > band.bottom() + kEpsilon) {
        continue;
      }
      const double weight = rawWeight(c, r, x, y);
      candidates.append({c, r, weight});
      total += weight;
    }
  }
  if (total <= kEpsilon) {
    return false;
  }
  double cumulative = 0;
  for (const Candidate& candidate : candidates) {
    cumulative += candidate.weight / total;
    if (target <= cumulative + kEpsilon) {
      return candidate.col == blend_.col && candidate.row == blend_.row;
    }
  }
  return false;
}

LaserRasterGalvoFactory::LaserRasterGalvoFactory(
    const FactoryKwargs& kwargs) noexcept
    : BaseFactory(kwargs) {
  qInfo() << "LaserRasterGalvoFactory created";
}

void LaserRasterGalvoFactory::add_bitmap(const QImage& gray,
                                         const QRectF& bbox_px,
                                         bool dots) {
  if (gray.isNull() || bbox_px.isEmpty()) {
    return;
  }
  Entry entry;
  entry.gray = gray.format() == QImage::Format_Grayscale8
                   ? gray
                   : gray.convertToFormat(QImage::Format_Grayscale8);
  entry.bbox_px = bbox_px;
  entry.dots = dots;
  bitmaps_.append(entry);
}

QRectF LaserRasterGalvoFactory::get_bounds_mm() const {
  QRectF box;
  for (const Entry& entry : bitmaps_) {
    const QRectF mm(entry.bbox_px.left() / pixel_per_mm_x,
                    entry.bbox_px.top() / pixel_per_mm,
                    entry.bbox_px.width() / pixel_per_mm_x,
                    entry.bbox_px.height() / pixel_per_mm);
    box = box.united(mm);
  }
  return box.isEmpty() ? box : box.translated(-offset);
}

void LaserRasterGalvoFactory::setPwm(float pwm) {
  if (current_pwm_ == pwm) {
    return;
  }
  current_pwm_ = pwm;
  proc->set_toolhead_pwm(pwm);
}

void LaserRasterGalvoFactory::emitLine(const Entry& entry,
                                       int index,
                                       int threshold,
                                       bool reversed,
                                       bool transposed,
                                       float speed) {
  const int width = entry.gray.width();
  const int height = entry.gray.height();
  // Along the row, or down the column when transposed.
  const int length = transposed ? height : width;
  const QImage& gray = entry.gray;
  auto sample = [&](int at) {
    return transposed ? gray.constScanLine(at)[index]
                      : gray.constScanLine(index)[at];
  };
  // Sample at pixel centres, the way the gcode raster does, so a run covers the
  // dots it is made of rather than stopping half a pixel short of the last one.
  const double fixed = transposed
                           ? (entry.bbox_px.left() + index + 0.5) /
                                 pixel_per_mm_x - offset.x()
                           : (entry.bbox_px.top() + index + 0.5) /
                                 pixel_per_mm - offset.y();
  auto moving = [&](int at) {
    return transposed
               ? (entry.bbox_px.top() + at) / pixel_per_mm - offset.y()
               : (entry.bbox_px.left() + at) / pixel_per_mm_x - offset.x();
  };
  auto move = [&](double along, bool travel, float feed) {
    NamedArgs args;
    if (transposed) {
      args.rx(fixed).ry(along);
    } else {
      args.rx(along).ry(fixed);
    }
    if (travel) {
      args.set_is_travel();
    } else {
      args.rf(feed);
    }
    proc->moveto(args);
  };

  if (entry.dots) {
    // One dot per dark pixel; the writer turns each of these marks into a jump
    // and a LASER_ON, since a dotting time is set for a dithered image.
    const double half_pixel =
        0.5 / (transposed ? pixel_per_mm : pixel_per_mm_x);
      for (int i = 0; i < length; i++) {
      const int at = reversed ? length - 1 - i : i;
      if (sample(at) >= threshold) {
        continue;
      }
      const double along = moving(at) + half_pixel;
      if (blend_.active) {
        const double x = transposed ? fixed : along;
        const double y = transposed ? along : fixed;
        if (!blend_band().contains(x, y)) {
          continue;  // beyond what this block samples
        }
        // The screen is indexed by the image's own pixel grid, which every
        // block walks identically, so all of them draw the same target for a
        // given site and agree on who owns it.
        const int column = transposed ? index : at;
        const int row = transposed ? at : index;
        if (!ownsSite(x, y, siteTarget(column, row))) {
          continue;
        }
      }
      setPwm(100);
      move(along, false, speed);
    }
    return;
  }

  // Runs of dark pixels, each one marked segment. Ownership cuts them the same
  // way a dark-to-light edge does, so a run simply ends where the neighbouring
  // block takes over.
  const QRectF run_band = blend_.active ? blend_band() : QRectF();
  auto owned = [&](int at) {
    if (!blend_.active) {
      return true;
    }
    const double along = moving(at) + 0.5 / (transposed ? pixel_per_mm : pixel_per_mm_x);
    const double px = transposed ? fixed : along;
    const double py = transposed ? along : fixed;
    return run_band.contains(px, py) && ownsSite(px, py, runTarget(px, py));
  };
  int run_start = -1;
  for (int i = 0; i <= length; i++) {
    const bool on = i < length && sample(i) < threshold && owned(i);
    if (on && run_start < 0) {
      run_start = i;
    } else if (!on && run_start >= 0) {
      // +1 on the end: the run covers the whole of its last pixel.
      double from = moving(run_start);
      double to = moving(i);
      if (reversed) {
        std::swap(from, to);
      }
      setPwm(0);
      move(from, true, 0);
      setPwm(100);
      move(to, false, speed);
      run_start = -1;
    }
  }
}

void LaserRasterGalvoFactory::generate_task_code(float speed, int threshold,
                                                 bool transposed) {
  if (bitmaps_.isEmpty()) {
    return;
  }
  current_pwm_ = 0;
  const QRectF hard_clip =
      proc->galvo() ? proc->galvo()->clip_rect() : QRectF();
  for (const Entry& entry : bitmaps_) {
    if (cancelled) {
      return;
    }
    // Dotting is a property of the image, not the layer: a dithered one is dots
    // and a binarised one is runs, and a layer can hold both. Both work past
    // their cell into the seam when blending, so the clip has to let the band
    // through and the block's own hard edge goes back afterwards.
    if (proc->galvo()) {
      proc->galvo()->params().dotting_time_us =
          entry.dots ? dotting_time_us_ : 0;
      if (blend_.active) {
        proc->galvo()->set_clip_rect(hard_clip.united(blend_band()));
      } else {
        proc->galvo()->set_clip_rect(hard_clip);
      }
    }
    const int lines = transposed ? entry.gray.width() : entry.gray.height();
    bool reversed = false;
    for (int line = 0; line < lines; line++) {
      if (cancelled) {
        return;
      }
      emitLine(entry, line, threshold, reversed, transposed, speed);
      // Alternate direction so the beam does not fly back for every line.
      reversed = !reversed;
    }
  }
  setPwm(0);
  if (proc->galvo()) {
    proc->galvo()->params().dotting_time_us = 0;
    proc->galvo()->set_clip_rect(hard_clip);
  }
}
