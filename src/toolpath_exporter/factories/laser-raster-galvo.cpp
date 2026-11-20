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
  switch (activeProfile()) {
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

double LaserRasterGalvoFactory::activeOverlap() const {
  return dot_pass_ ? blend_.dot_overlap : blend_.line_overlap;
}

LaserRasterGalvoFactory::BlendProfile LaserRasterGalvoFactory::activeProfile() const {
  return dot_pass_ ? blend_.dot_profile : blend_.line_profile;
}

bool LaserRasterGalvoFactory::blendActive() const {
  if (!blend_.active) {
    return false;
  }
  // The hybrid's run pass is bounded by the core, which is cut from the dot
  // overlap, so that is what has to be non-zero for it -- the line knobs take
  // no part in the hybrid at all.
  if (dot_pass_ || blend_.band_dots) {
    return blend_.dot_overlap > 0;
  }
  return blend_.line_overlap > 0;
}

QRectF LaserRasterGalvoFactory::bandAt(int col, int row) const {
  const double half = activeOverlap() / 2;
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
  // ground two blocks both reach. Measured off the dot overlap whichever pass
  // asks, because the hybrid this serves is a dot band from end to end.
  const double half = blend_.dot_overlap / 2;
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
  const double overlap = activeOverlap();

  return profileDensity(axisProgress(x, cell.left(), cell.right(), overlap,
                                     col > blend_.min_col,
                                     col < blend_.max_col)) *
         profileDensity(axisProgress(y, cell.top(), cell.bottom(), overlap,
                                     row > blend_.min_row,
                                     row < blend_.max_row));
}

double LaserRasterGalvoFactory::siteTarget(int column, int row) const {
  const int sx = ((column % kScreenWidth) + kScreenWidth) % kScreenWidth;
  const int sy = ((row % kScreenHeight) + kScreenHeight) % kScreenHeight;
  return (kScreen[sy][sx] + 0.5) / kScreenPhases;
}

double LaserRasterGalvoFactory::dotTarget(double x,
                                          double y,
                                          int column,
                                          int row) const {
  const double grain = blend_.dot_grain;
  if (grain <= 0) {
    return siteTarget(column, row);
  }
  // The same ordered screen, one phase per square instead of per site, so
  // neighbouring squares spread their owners as evenly as sites do.
  return siteTarget(int(std::floor(x / grain)), int(std::floor(y / grain)));
}

double LaserRasterGalvoFactory::runTarget(double along, int line) const {
  // Which of the three line strategies this is, in the simulator's terms: the
  // phase is built from the stretch along the scan, from the scan line, or from
  // both.
  long long phase = 0;
  if (blend_.line_emission != RunEmission::Scanlines) {
    const double length = blend_.segment_length > 0 ? blend_.segment_length : 2;
    phase += static_cast<long long>(std::floor(along / length)) *
             kRunSegmentStride;
  }
  if (blend_.line_emission != RunEmission::Segments) {
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
                                         bool dots,
                                         const QImage& source) {
  if (gray.isNull() || bbox_px.isEmpty()) {
    return;
  }
  Entry entry;
  if (!source.isNull() && source.size() == gray.size()) {
    entry.source = source.format() == QImage::Format_Grayscale8
                       ? source
                       : source.convertToFormat(QImage::Format_Grayscale8);
  }
  seams_.reset();  // built from the images; one more changes them
  entry.gray = gray.format() == QImage::Format_Grayscale8
                   ? gray
                   : gray.convertToFormat(QImage::Format_Grayscale8);
  // On the whole-pixel grid, so images can be composited pixel for pixel. The
  // shift is under half a pixel.
  entry.bbox_px = QRectF(std::round(bbox_px.left()), std::round(bbox_px.top()),
                         entry.gray.width(), entry.gray.height());
  entry.dots = dots;
  bitmaps_.append(entry);
}

QVector<LaserRasterGalvoFactory::Entry> LaserRasterGalvoFactory::mergedEntries(
    const QRect& region_px) const {
  auto rect_of = [](const Entry& entry) {
    return QRect(int(entry.bbox_px.left()), int(entry.bbox_px.top()),
                 entry.gray.width(), entry.gray.height());
  };
  // Kinds in the order their first image was added, so a layer keeps the
  // order it had between its runs and its dots.
  QVector<bool> kinds;
  for (const Entry& entry : bitmaps_) {
    if (!kinds.contains(entry.dots)) {
      kinds.append(entry.dots);
    }
  }
  QVector<Entry> merged;
  for (bool dots : kinds) {
    QVector<const Entry*> group;
    QRect area;
    for (const Entry& entry : bitmaps_) {
      if (entry.dots != dots) {
        continue;
      }
      const QRect rect = rect_of(entry);
      if (!region_px.isEmpty() && !rect.intersects(region_px)) {
        continue;  // the block's clip would drop all of it anyway
      }
      group.append(&entry);
      area = area.united(rect);
    }
    if (group.isEmpty()) {
      continue;
    }
    if (group.size() == 1) {
      merged.append(*group.first());
      continue;
    }
    if (!region_px.isEmpty()) {
      area = area.intersected(region_px);
    }
    if (area.isEmpty()) {
      continue;
    }
    QImage gray(area.size(), QImage::Format_Grayscale8);
    gray.fill(255);
    // The sources go the same way, as long as every image brought one.
    const bool has_source =
        std::all_of(group.begin(), group.end(),
                    [](const Entry* e) { return !e->source.isNull(); });
    QImage source;
    if (has_source) {
      source = QImage(area.size(), QImage::Format_Grayscale8);
      source.fill(255);
    }
    auto darkest = [&](const QImage& from, const QRect& rect, QImage* into) {
      const QRect overlap = rect.intersected(area);
      for (int y = overlap.top(); y <= overlap.bottom(); y++) {
        const uchar* src = from.constScanLine(y - rect.top()) +
                           (overlap.left() - rect.left());
        uchar* dst =
            into->scanLine(y - area.top()) + (overlap.left() - area.left());
        for (int x = 0; x < overlap.width(); x++) {
          dst[x] = qMin(dst[x], src[x]);
        }
      }
    };
    for (const Entry* entry : group) {
      darkest(entry->gray, rect_of(*entry), &gray);
      if (has_source) {
        darkest(entry->source, rect_of(*entry), &source);
      }
    }
    Entry entry;
    entry.gray = gray;
    entry.bbox_px = QRectF(area);
    entry.dots = dots;
    entry.source = source;
    merged.append(entry);
  }
  return merged;
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
    // One dot per dark pixel, written as a packed run: a scan line is a
    // unidirectional sweep on a fixed pitch, which is what opcode 0x20 encodes
    // (docs/galvo-packed-dots.md). The site index is the place in the sweep, so
    // it counts up whichever way the line is scanned, and the step carries the
    // direction instead.
    const double half_pixel =
        0.5 / (transposed ? pixel_per_mm : pixel_per_mm_x);
    const double pitch = 1 / (transposed ? pixel_per_mm : pixel_per_mm_x);
    GalvoListWriter* galvo = proc->galvo();
    if (galvo) {
      const double base_along = moving(reversed ? length - 1 : 0) + half_pixel;
      const double step_along = reversed ? -pitch : pitch;
      galvo->beginDotRun(transposed ? QPointF(fixed, base_along)
                                    : QPointF(base_along, fixed),
                         transposed ? QPointF(0, step_along)
                                    : QPointF(step_along, 0),
                         galvo->params().dotting_time_us);
    }
    for (int i = 0; i < length; i++) {
      const int at = reversed ? length - 1 - i : i;
      if (sample(at) >= threshold) {
        continue;
      }
      const double along = moving(at) + half_pixel;
      // The machine's pixel grid, which every block walks identically, so all
      // of them agree on who owns a site -- even when one block walks the
      // image on its own and its neighbour walks it merged with another.
      const int column = int(entry.bbox_px.left()) + (transposed ? index : at);
      const int row = int(entry.bbox_px.top()) + (transposed ? at : index);
      if (active_seams_) {
        // K-pass walk: the seams say outright which cell lays this site.
        if (blend_.active) {
          int owner_col = 0;
          int owner_row = 0;
          active_seams_->owner(column, row, &owner_col, &owner_row);
          if (owner_col != blend_.col || owner_row != blend_.row) {
            continue;
          }
        }
      } else if (blendActive()) {
        const double x = transposed ? fixed : along;
        const double y = transposed ? along : fixed;
        if (!blend_band().contains(x, y)) {
          continue;  // beyond what this block samples
        }
        // The screen is indexed by that same grid, so every block draws the
        // same target for a given site.
        if (!ownsSite(x, y, dotTarget(x, y, column, row))) {
          continue;
        }
      }
      if (galvo && blend_.blank_jump_wait > 0 && !transposed &&
          !entry.source.isNull()) {
        // The beam is about to leave the dot before this one. If the way here
        // crosses a blank of the picture, it waits there first.
        if (prev_dot_valid_ &&
            blankBetween(entry, prev_dot_x_, prev_dot_y_, at, index)) {
          galvo->dwell(blend_.blank_jump_wait);
        }
        prev_dot_valid_ = true;
        prev_dot_x_ = at;
        prev_dot_y_ = index;
      }
      setPwm(100);
      if (galvo) {
        galvo->dotAt(i);
      } else {
        move(along, false, speed);
      }
    }
    if (galvo) {
      galvo->endDotRun();
    }
    return;
  }

  // Runs of dark pixels, each one marked segment. Ownership cuts them the same
  // way a dark-to-light edge does, so a run simply ends where the neighbouring
  // block takes over.
  const QRectF run_band = blendActive() ? blend_band() : QRectF();
  // The hybrid leaves the band to its dot pass, so the runs stop at the core.
  const QRectF run_region =
      blendActive() && blend_.band_dots ? coreRect() : run_band;
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
      blend_.line_emission == RunEmission::Segments && (global_line & 1) != 0;
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
    if (!blendActive()) {
      return 1;
    }
    const QPointF p = site(at);
    if (!run_region.contains(p)) {
      return 0;
    }
    if (blend_.band_dots) {
      return 1;  // the core is full exposure; the band is the dot pass's
    }
    if (blend_.line_emission == RunEmission::Pwm) {
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

std::shared_ptr<const GalvoKpassSeams> LaserRasterGalvoFactory::kpassSeams() {
  if (!blend_.kpass.enabled) {
    return nullptr;
  }
  // One block is a lattice of one: no seams, but the passes are still made.
  const int lattice[4] = {
      blend_.active ? blend_.min_col : blend_.col,
      blend_.active ? blend_.max_col : blend_.col,
      blend_.active ? blend_.min_row : blend_.row,
      blend_.active ? blend_.max_row : blend_.row,
  };
  if (seams_ && std::equal(lattice, lattice + 4, seams_lattice_) &&
      seams_step_ == blend_.step) {
    return seams_;
  }
  QVector<GalvoKpassSeams::Image> images;
  for (const Entry& entry : bitmaps_) {
    if (!entry.dots) {
      continue;
    }
    images.append({QRect(int(entry.bbox_px.left()), int(entry.bbox_px.top()),
                         entry.gray.width(), entry.gray.height()),
                   &entry.gray,
                   entry.source.isNull() ? nullptr : &entry.source});
  }
  if (images.isEmpty()) {
    return nullptr;
  }
  GalvoKpassSeams::Lattice l;
  l.step = blend_.step;
  l.min_col = lattice[0];
  l.max_col = lattice[1];
  l.min_row = lattice[2];
  l.max_row = lattice[3];
  l.half_field = blend_.kpass.half_field;
  l.px_per_mm_x = pixel_per_mm_x;
  l.px_per_mm_y = pixel_per_mm;
  l.offset = offset;
  seams_ = GalvoKpassSeams::build(blend_.kpass.params, l, images);
  std::copy(lattice, lattice + 4, seams_lattice_);
  seams_step_ = blend_.step;
  return seams_;
}

QImage LaserRasterGalvoFactory::renderDots(const QRect& range_px,
                                           int shrink) const {
  shrink = qMax(1, shrink);
  const int w = qMax(1, range_px.width() / shrink);
  const int h = qMax(1, range_px.height() / shrink);
  QVector<int> count(w * h, 0);
  for (const Entry& entry : bitmaps_) {
    if (!entry.dots) {
      continue;
    }
    const QRect rect(int(entry.bbox_px.left()), int(entry.bbox_px.top()),
                     entry.gray.width(), entry.gray.height());
    const QRect part = rect.intersected(range_px);
    for (int y = part.top(); y <= part.bottom(); y++) {
      const uchar* line = entry.gray.constScanLine(y - rect.top());
      const int oy = (y - range_px.top()) / shrink;
      if (oy >= h) {
        break;
      }
      for (int x = part.left(); x <= part.right(); x++) {
        const int ox = (x - range_px.left()) / shrink;
        if (ox < w && line[x - rect.left()] != 255) {
          count[oy * w + ox]++;
        }
      }
    }
  }
  QImage out(w, h, QImage::Format_Grayscale8);
  const double full = shrink * shrink;
  for (int y = 0; y < h; y++) {
    uchar* line = out.scanLine(y);
    for (int x = 0; x < w; x++) {
      line[x] = uchar(255 - qMin(255.0, 255.0 * count[y * w + x] / full));
    }
  }
  return out;
}

bool LaserRasterGalvoFactory::blankBetween(const Entry& entry, int x0, int y0,
                                           int x1, int y1) const {
  const double pitch_x = 1 / pixel_per_mm_x;
  const double pitch_y = 1 / pixel_per_mm;
  const double min_mm = blend_.blank_jump_min;
  const QImage& source = entry.source;
  if (y0 == y1) {
    // Along a row: the pixels strictly between the two dots.
    const int lo = qMin(x0, x1);
    const int hi = qMax(x0, x1);
    if ((hi - lo) * pitch_x <= min_mm) {
      return false;
    }
    const uchar* line = source.constScanLine(y0);
    int run = 0;
    for (int x = lo + 1; x < hi; x++) {
      run = line[x] == 255 ? run + 1 : 0;
      if (run * pitch_x >= min_mm - kEpsilon) {
        return true;
      }
    }
    return false;
  }
  // From the end of one row to the start of another: the straight line the
  // jump takes, sampled twice per pixel.
  const double length =
      std::hypot((x1 - x0) * pitch_x, (y1 - y0) * pitch_y);
  if (length <= min_mm) {
    return false;
  }
  const int samples =
      qMax(3, int(std::hypot(double(x1 - x0), double(y1 - y0)) * 2));
  const double per_sample = length / (samples - 1);
  int run = 0;
  for (int k = 1; k < samples - 1; k++) {
    const double t = double(k) / (samples - 1);
    const int x = qBound(0, qRound(x0 + (x1 - x0) * t), source.width() - 1);
    const int y = qBound(0, qRound(y0 + (y1 - y0) * t), source.height() - 1);
    run = source.constScanLine(y)[x] == 255 ? run + 1 : 0;
    if (run * per_sample >= min_mm - kEpsilon) {
      return true;
    }
  }
  return false;
}

bool LaserRasterGalvoFactory::sampleAt(const Entry& entry,
                                      const QPointF& mm,
                                      int threshold) const {
  const int x = int(std::floor((mm.x() + offset.x()) * pixel_per_mm_x -
                               entry.bbox_px.left()));
  const int y = int(std::floor((mm.y() + offset.y()) * pixel_per_mm -
                               entry.bbox_px.top()));
  if (x < 0 || y < 0 || x >= entry.gray.width() || y >= entry.gray.height()) {
    return false;
  }
  return entry.gray.constScanLine(y)[x] < threshold;
}

void LaserRasterGalvoFactory::emitBandDots(const Entry& entry,
                                           int threshold,
                                           float speed) {
  const QRectF band = blend_band();
  const QRectF core = coreRect();
  if (band.isEmpty()) {
    return;
  }
  // The pitch the dots are laid on. Falling back to the image's own is what
  // keeps a caller who names nothing where they were before.
  const double pitch_x = blend_.dot_process.pitch_mm > 0
                             ? blend_.dot_process.pitch_mm
                             : 1 / pixel_per_mm_x;
  const double pitch_y = blend_.dot_process.pitch_mm > 0
                             ? blend_.dot_process.pitch_mm
                             : 1 / pixel_per_mm;
  if (pitch_x <= 0 || pitch_y <= 0) {
    return;
  }
  // Site centres sit half a pitch in, on a grid measured from the machine
  // origin, so two blocks sampling the same seam sample the same places.
  const int first_col = int(std::ceil(band.left() / pitch_x - 0.5 - kEpsilon));
  const int last_col = int(std::floor((band.right() - kEpsilon) / pitch_x - 0.5));
  const int first_row = int(std::ceil(band.top() / pitch_y - 0.5 - kEpsilon));
  const int last_row = int(std::floor((band.bottom() - kEpsilon) / pitch_y - 0.5));

  GalvoListWriter* galvo = proc->galvo();
  setPower(base_power_pct_);
  for (int row = first_row; row <= last_row; row++) {
    if (cancelled) {
      return;
    }
    const double y = (row + 0.5) * pitch_y;
    QVector<int> cols;
    for (int col = first_col; col <= last_col; col++) {
      const double x = (col + 0.5) * pitch_x;
      if (core.contains(x, y)) {
        continue;  // the run pass has already covered the core at full power
      }
      if (!sampleAt(entry, QPointF(x, y), threshold)) {
        continue;
      }
      if (!ownsSite(x, y, dotTarget(x, y, col, row))) {
        continue;
      }
      cols.append(col);
    }
    if (cols.isEmpty()) {
      continue;
    }
    // Serpentine on the row's own index plus the block's, as the simulator
    // does, so the sweep carries on in the direction it arrived.
    const bool reversed = ((row + blend_.row) & 1) != 0;
    if (reversed) {
      std::reverse(cols.begin(), cols.end());
    }
    // A row is one sweep on a fixed pitch, so it is one packed run; the
    // columns are already in sweep order, which makes the site index the
    // distance from the first of them.
    const int start_col = cols.first();
    if (galvo) {
      galvo->beginDotRun(QPointF((start_col + 0.5) * pitch_x, y),
                         QPointF(reversed ? -pitch_x : pitch_x, 0),
                         galvo->params().dotting_time_us);
    }
    for (int col : cols) {
      setPwm(100);
      if (galvo) {
        galvo->dotAt(reversed ? start_col - col : col - start_col);
      } else {
        proc->moveto(NamedArgs().rx((col + 0.5) * pitch_x).ry(y).rf(speed));
      }
    }
    if (galvo) {
      galvo->endDotRun();
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
  // The layer's power, as the prologue already carries it. A pwm fade takes
  // shares of this and has to be able to put it back.
  base_power_pct_ = proc->galvo() ? proc->galvo()->params().power_pct : 0;
  current_power_pct_ = base_power_pct_;
  // A run pass and a dot pass over the same image, which is the hybrid seam:
  // the core at full exposure as runs, the band around it as blended dots.
  const bool two_pass = blend_.active && blend_.band_dots && blend_.dot_overlap > 0;
  // Everything this block can sample: its clip, grown by the wider of the two
  // seam bands, and a pixel more for rounding. A block that is not clipped --
  // the whole layer fits one field -- takes every image whole.
  // The K-pass walk, when the layer asks for it, is how dithered images are
  // laid; a transposed pass is a depth one and has nothing to do with it.
  const std::shared_ptr<const GalvoKpassSeams> seams =
      blend_.kpass.enabled && !transposed ? kpassSeams() : nullptr;
  // How far past its clip this block is handed dots, per axis. Under the
  // K-pass walk that is the whole of its field: a seam strays no further than
  // the field reaches (GalvoKpassSeams fits it), and a neighbour left with a
  // handful of dots hands them over from anywhere this field covers.
  const double dot_reach_x =
      !blend_.active
          ? 0
          : seams ? qMax(seams->reachX(), blend_.kpass.half_field -
                                              blend_.step.width() / 2)
                  : blend_.dot_overlap / 2;
  const double dot_reach_y =
      !blend_.active
          ? 0
          : seams ? qMax(seams->reachY(), blend_.kpass.half_field -
                                              blend_.step.height() / 2)
                  : blend_.dot_overlap / 2;
  QRect region_px;
  if (!hard_clip.isEmpty()) {
    const double line_reach = blend_.active ? blend_.line_overlap / 2 : 0;
    const double reach_x = qMax(dot_reach_x, line_reach);
    const double reach_y = qMax(dot_reach_y, line_reach);
    const QRectF region_mm =
        hard_clip.adjusted(-reach_x, -reach_y, reach_x, reach_y);
    region_px = QRectF((region_mm.left() + offset.x()) * pixel_per_mm_x,
                       (region_mm.top() + offset.y()) * pixel_per_mm,
                       region_mm.width() * pixel_per_mm_x,
                       region_mm.height() * pixel_per_mm)
                    .toAlignedRect()
                    .adjusted(-1, -1, 1, 1);
  }
  const QVector<Entry> entries = mergedEntries(region_px);
  for (const Entry& entry : entries) {
    if (cancelled) {
      return;
    }
    // Dotting is a property of the image, not the layer: a dithered one is dots
    // and a binarised one is runs, and a layer can hold both. Which band this
    // pass works to follows from that, so the flag has to be set before
    // anything asks -- the clip below asks first.
    dot_pass_ = entry.dots;
    const bool kpass = seams && entry.dots;
    prev_dot_valid_ = false;  // the wait before a jump never spans two images
    // Both work past their cell into the seam when blending, so the clip has to
    // let the band through and the block's own hard edge goes back afterwards.
    if (proc->galvo()) {
      proc->galvo()->params().dotting_time_us =
          entry.dots ? dotting_time_us_ : 0;
      if (kpass && blend_.active && !hard_clip.isEmpty()) {
        // The seams decide who lays what; the clip only has to stay out of the
        // way as far as they can hand this block a dot.
        proc->galvo()->set_clip_rect(hard_clip.adjusted(
            -dot_reach_x, -dot_reach_y, dot_reach_x, dot_reach_y));
      } else if (blendActive()) {
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
    if (kpass) {
      // K passes over the stop, pass p laying the rows that are p modulo K on
      // the machine grid, in the direction that pass is given; within a pass
      // the rows still alternate left and right.
      active_seams_ = seams.get();
      const int passes = seams->passes();
      for (int pass = 0; pass < passes; pass++) {
        if (pass > 0 && blend_.kpass.pass_pause > 0 && proc->galvo()) {
          setPwm(0);
          proc->galvo()->pause(blend_.kpass.pass_pause);
          current_pwm_ = 0;
          prev_dot_valid_ = false;  // the beam has rested: no tail to wait out
        }
        const bool up = seams->passUp(pass);
        for (int n = 0; n < lines; n++) {
          if (cancelled) {
            active_seams_ = nullptr;
            return;
          }
          const int line = up ? lines - 1 - n : n;
          const int grid_row = first_line + line;
          if (seams->passOf(grid_row) != pass) {
            continue;
          }
          const int turn = (grid_row - seams->passOf(grid_row)) / passes;
          emitLine(entry, line, threshold, (turn & 1) != 0, transposed, speed);
        }
      }
      active_seams_ = nullptr;
      continue;
    }
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
    // Second pass: the band the runs stopped short of, laid as dots.
    //
    // It runs in a list of its own. A galvo list states its speeds, powers and
    // delays in its prologue and nowhere else, so giving the band a different
    // power or a different jump speed means giving it a different list -- which
    // is also how the simulator does it, the hybrid returning the line blocks
    // and the dot blocks as two separate lists.
    dot_pass_ = true;
    GalvoParams saved;
    if (proc->galvo()) {
      GalvoListWriter* galvo = proc->galvo();
      galvo->endList();
      saved = galvo->params();
      GalvoParams& dot = galvo->params();
      const Blend::DotProcess& wanted = blend_.dot_process;
      if (wanted.power_pct > 0) {
        dot.power_pct = wanted.power_pct;
      }
      if (wanted.pulse_period_us > 0) {
        dot.pulse_period_us = wanted.pulse_period_us;
      }
      if (wanted.jump_speed_mm_s > 0) {
        dot.jump_speed_mm_s = wanted.jump_speed_mm_s;
      }
      if (!qIsNaN(wanted.laser_on_delay_us)) {
        dot.laser_on_delay_us = wanted.laser_on_delay_us;
      }
      if (!qIsNaN(wanted.laser_off_delay_us)) {
        dot.laser_off_delay_us = wanted.laser_off_delay_us;
      }
      // A dot's dwell stands in for the time a line pass would have spent over
      // the same ground, so the band reads at the same depth as the core it
      // meets -- unless the caller has measured something better.
      const double pitch =
          wanted.pitch_mm > 0 ? wanted.pitch_mm : 1 / pixel_per_mm_x;
      dot.dotting_time_us = wanted.pulse_on_time_us > 0
                                ? wanted.pulse_on_time_us
                                : (speed > 0 ? pitch / (speed / 60) * 1e6 : 0);
      galvo->set_clip_rect(hard_clip.united(blend_band()));
      galvo->beginList();
      // The list's own power is the one its prologue just stated; a fade has
      // nothing to do here, but setPower still has to agree with it.
      base_power_pct_ = dot.power_pct;
      current_power_pct_ = dot.power_pct;
      current_pwm_ = 0;
    }
    emitBandDots(entry, threshold, speed);
    if (proc->galvo()) {
      GalvoListWriter* galvo = proc->galvo();
      setPwm(0);
      galvo->endList();
      galvo->params() = saved;
      galvo->beginList();
      base_power_pct_ = saved.power_pct;
      current_power_pct_ = saved.power_pct;
      current_pwm_ = 0;
    }
  }
  dot_pass_ = false;
  setPwm(0);
  setPower(base_power_pct_);
  if (proc->galvo()) {
    proc->galvo()->params().dotting_time_us = 0;
    proc->galvo()->set_clip_rect(hard_clip);
  }
}
