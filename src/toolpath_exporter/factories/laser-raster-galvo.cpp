#include "laser-raster-galvo.h"

#include <QDebug>
#include <QtGlobal>
#include <algorithm>
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
// Ownership phases for runs. The two multipliers below are coprime with it, so
// neighbouring stretches and neighbouring lines land far apart in the ordering.
constexpr int kRunPhases = 100;
constexpr int kRunSegmentStride = 61;
constexpr int kRunLineStride = 37;

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

QRectF LaserRasterGalvoFactory::coreRect() const {
  // The mirror image of the band: where the band grows into a shared seam, the
  // core draws back from it by the same half. Between them lies exactly the
  // ground two blocks both reach.
  const double half = blend_.overlap / 2;
  return cellAt(blend_.col, blend_.row)
      .adjusted(blend_.col > blend_.min_col ? half : 0,
                blend_.row > blend_.min_row ? half : 0,
                blend_.col < blend_.max_col ? -half : 0,
                blend_.row < blend_.max_row ? -half : 0);
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

double LaserRasterGalvoFactory::runTarget(double along, int line) const {
  // Which of the three line strategies this is, in the simulator's terms: the
  // phase is built from the stretch along the scan, from the scan line, or from
  // both.
  long long phase = 0;
  if (blend_.run_emission != RunEmission::Scanlines) {
    const double length = blend_.segment_length > 0 ? blend_.segment_length : 2;
    phase += static_cast<long long>(std::floor(along / length)) *
             kRunSegmentStride;
  }
  if (blend_.run_emission != RunEmission::Segments) {
    phase += static_cast<long long>(line) * kRunLineStride;
  }
  phase %= kRunPhases;
  if (phase < 0) {
    phase += kRunPhases;
  }
  return (phase + 0.5) / kRunPhases;
}

double LaserRasterGalvoFactory::collectCandidates(double x,
                                                  double y,
                                                  QVector<Candidate>* out) const {
  const double sx = blend_.step.width();
  const double sy = blend_.step.height();
  const int near_col = int(std::floor(x / sx + 0.5));
  const int near_row = int(std::floor(y / sy + 0.5));
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
      out->append({c, r, weight});
      total += weight;
    }
  }
  return total;
}

double LaserRasterGalvoFactory::blockWeight(double x, double y) const {
  QVector<Candidate> candidates;
  const double total = collectCandidates(x, y, &candidates);
  if (total <= kEpsilon) {
    return 0;
  }
  for (const Candidate& candidate : candidates) {
    if (candidate.col == blend_.col && candidate.row == blend_.row) {
      return candidate.weight / total;
    }
  }
  return 0;
}

bool LaserRasterGalvoFactory::ownsSite(double x,
                                       double y,
                                       double target,
                                       bool reversed) const {
  QVector<Candidate> candidates;
  const double total = collectCandidates(x, y, &candidates);
  if (total <= kEpsilon) {
    return false;
  }
  if (reversed) {
    std::reverse(candidates.begin(), candidates.end());
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

void LaserRasterGalvoFactory::setPower(double pct) {
  if (current_power_pct_ == pct) {
    return;
  }
  current_power_pct_ = pct;
  // A negative pwm is how the toolhead interface carries a power rather than an
  // on/off: the writer turns it into SET_LASER_POWER, and on a CO2 head into
  // the pulse length that actually holds the duty.
  proc->set_toolhead_pwm(-pct / 100);
  current_pwm_ = 0;  // the laser-on state has to be restated after this
}

void LaserRasterGalvoFactory::emitLine(const Entry& entry,
                                       int index,
                                       int threshold,
                                       bool reversed,
                                       bool transposed,
                                       float speed,
                                       bool force_dots) {
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

  if (entry.dots || force_dots) {
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
        if (force_dots && coreRect().contains(x, y)) {
          continue;  // the run pass has already covered the core at full power
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
  // The hybrid leaves the band to its dot pass, so the runs stop at the core.
  const QRectF run_region =
      blend_.active && blend_.band_dots ? coreRect() : run_band;
  // The scan line's index on the machine's own grid rather than this bitmap's,
  // so two blocks -- and two bitmaps in one layer -- draw the same phase for
  // the same line.
  const int global_line =
      qRound(transposed ? entry.bbox_px.left() : entry.bbox_px.top()) + index;
  // Segments builds no line term into the phase, so on its own the teeth would
  // stand in the same places on every line. What alternates them is the walk
  // running the other way down the candidates on the lines that are themselves
  // scanned backwards.
  const bool reverse_candidates =
      blend_.run_emission == RunEmission::Segments && (global_line & 1) != 0;
  const double half_pixel_along =
      0.5 / (transposed ? pixel_per_mm : pixel_per_mm_x);
  auto site = [&](int at) {
    const double along = moving(at) + half_pixel_along;
    return QPointF(transposed ? fixed : along, transposed ? along : fixed);
  };
  // What share of this site falls to this block: one where it is the only one
  // that reaches, zero where the seam rule hands it elsewhere, and a fraction
  // in between only when the band is faded rather than divided.
  auto share = [&](int at) -> double {
    if (!blend_.active) {
      return 1;
    }
    const QPointF p = site(at);
    if (!run_region.contains(p)) {
      return 0;
    }
    if (blend_.band_dots) {
      return 1;  // the core is full exposure; the band is the dot pass's
    }
    if (blend_.run_emission == RunEmission::Pwm) {
      return blockWeight(p.x(), p.y());
    }
    const double along = moving(at) + half_pixel_along;
    return ownsSite(p.x(), p.y(), runTarget(along, global_line),
                    reverse_candidates)
               ? 1
               : 0;
  };

  // A run breaks wherever the share changes, not only where it falls to zero:
  // a fade crosses the band as a handful of pieces, each marked at its own
  // power, because the profile it is drawn from is a staircase. Pieces are
  // collected first so that a backwards line can emit them back to front and
  // the beam still travels the way it scans.
  struct Piece {
    int from;  // first pixel
    int to;    // one past the last, so the run covers all of it
    double share;
  };
  QVector<Piece> pieces;
  int run_start = -1;
  double run_share = 0;
  for (int i = 0; i <= length; i++) {
    const double s = i < length && sample(i) < threshold ? share(i) : 0;
    if (run_start >= 0 && (s <= 0 || qAbs(s - run_share) > kEpsilon)) {
      pieces.append({run_start, i, run_share});
      run_start = -1;
    }
    if (s > 0 && run_start < 0) {
      run_start = i;
      run_share = s;
    }
  }

  int previous_end = -1;
  for (int p = 0; p < pieces.size(); p++) {
    const Piece& piece = pieces[reversed ? pieces.size() - 1 - p : p];
    const int start = reversed ? piece.to : piece.from;
    const int end = reversed ? piece.from : piece.to;
    // Where one piece ends the next begins, so a fade is one unbroken sweep
    // and only the power changes under it. Jumping between them would leave
    // the beam dwelling at every step of the staircase.
    if (start != previous_end) {
      setPwm(0);
      move(moving(start), true, 0);
    }
    setPower(base_power_pct_ * piece.share);
    setPwm(100);
    move(moving(end), false, speed);
    previous_end = end;
  }
  setPower(base_power_pct_);
}

void LaserRasterGalvoFactory::generate_task_code(float speed, int threshold,
                                                 bool transposed) {
  if (bitmaps_.isEmpty()) {
    return;
  }
  current_pwm_ = 0;
  const QRectF hard_clip =
      proc->galvo() ? proc->galvo()->clip_rect() : QRectF();
  // The layer's power, as the prologue already carries it. A pwm fade takes
  // shares of this and has to be able to put it back.
  base_power_pct_ = proc->galvo() ? proc->galvo()->params().power_pct : 0;
  current_power_pct_ = base_power_pct_;
  // A run pass and a dot pass over the same image, which is the hybrid seam:
  // the core at full exposure as runs, the band around it as blended dots.
  const bool two_pass = blend_.active && blend_.band_dots;
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
    // Which way a line is scanned follows its index on the machine grid, not
    // its place in this bitmap. Runs decide ownership partly from that parity,
    // so it has to mean the same thing to every block -- and alternating anyway
    // is what keeps the beam from flying back for every line.
    const int first_line =
        qRound(transposed ? entry.bbox_px.left() : entry.bbox_px.top());
    for (int line = 0; line < lines; line++) {
      if (cancelled) {
        return;
      }
      const bool reversed = ((first_line + line) & 1) != 0;
      emitLine(entry, line, threshold, reversed, transposed, speed);
    }
    if (!two_pass || entry.dots) {
      continue;
    }
    // Second pass: the band the runs stopped short of, laid as dots. A dot's
    // dwell stands in for the time a line pass would have spent over the same
    // ground, so the band reads at the same depth as the core it meets.
    if (proc->galvo()) {
      const double pitch =
          1 / (transposed ? pixel_per_mm : pixel_per_mm_x);
      proc->galvo()->params().dotting_time_us =
          blend_.band_dot_time_us > 0
              ? blend_.band_dot_time_us
              : (speed > 0 ? pitch / (speed / 60) * 1e6 : 0);
    }
    for (int line = 0; line < lines; line++) {
      if (cancelled) {
        return;
      }
      const bool reversed = ((first_line + line) & 1) != 0;
      emitLine(entry, line, threshold, reversed, transposed, speed, true);
    }
  }
  setPwm(0);
  setPower(base_power_pct_);
  if (proc->galvo()) {
    proc->galvo()->params().dotting_time_us = 0;
    proc->galvo()->set_clip_rect(hard_clip);
  }
}
