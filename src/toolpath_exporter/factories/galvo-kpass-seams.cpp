#include "galvo-kpass-seams.h"

#include <QDebug>
#include <algorithm>
#include <cmath>
#include <limits>

namespace {

constexpr double kEpsilon = 1e-9;

// --- the slow walk ---------------------------------------------------------
// An AR(1) process: x' = a x + sigma sqrt(1 - a^2) n. Clipped at 3 sigma and
// scaled so that clip lands on the walk's range.
constexpr double kWalkSigma = 2.5;
constexpr double kWalkClip = 7.5;

// --- the cheapest path -----------------------------------------------------
// mm. Side of the cell the tone is averaged into before the path is sought.
constexpr double kCostCellMm = 0.25;
// What straying from the walk costs, per walk range strayed. Small beside any
// tone, so the image decides wherever it has an opinion and the walk decides
// where it has none.
constexpr double kCostPull = 0.05;
// What stepping one cell sideways between two rows costs.
constexpr double kCostTurn = 0.02;
// mm. The path is a moving average over this much seam.
constexpr double kPathSmoothMm = 2.0;
// mm. A strip of blank at least this wide costs nothing to run in, and closes
// the per-pass offsets up where they would otherwise land on ink.
constexpr double kBlankStripMm = 1.0;
// mm. The offsets open back up over this much seam ...
constexpr double kSpreadRampMm = 2.0;
// ... and are only closed for a stretch at least this long.
constexpr double kSpreadRunMm = 2.0;
// A cell left with fewer dots than this is not worth a stop of its own.
constexpr int kFewDots = 200;
// mm. How far inside the field's edge handed-over dots must stay.
constexpr double kHandoverMarginMm = 0.1;

/** A small generator of its own, so a seam is the same on every platform. */
class Random {
 public:
  explicit Random(quint64 seed) : state_(seed * 0x9E3779B97F4A7C15ULL + 1) {}
  double uniform() {  // (0, 1)
    state_ ^= state_ << 13;
    state_ ^= state_ >> 7;
    state_ ^= state_ << 17;
    return (double(state_ >> 11) + 0.5) / 9007199254740992.0;
  }
  double gauss() {
    const double u = uniform();
    const double v = uniform();
    return std::sqrt(-2 * std::log(u)) * std::cos(2 * M_PI * v);
  }

 private:
  quint64 state_;
};

QVector<float> slowWalk(int n, quint64 seed, double range_mm,
                        double pitch_mm, double length_mm) {
  QVector<float> out(n, 0.0f);
  if (range_mm <= 0 || n <= 0) {
    return out;
  }
  const double a =
      qBound(0.0, 1 - pitch_mm / qMax(length_mm, pitch_mm), 0.999999);
  const double b = kWalkSigma * std::sqrt(1 - a * a);
  Random rng(3000 + seed);
  double x = rng.gauss() * kWalkSigma;
  for (int k = 0; k < n; k++) {
    x = a * x + b * rng.gauss();
    out[k] = float(qBound(-kWalkClip, x, kWalkClip) * range_mm / kWalkClip);
  }
  return out;
}

/** Linear interpolation of `values`, sampled at `centres`, clamped at the ends. */
double interpolate(const QVector<double>& values, double first_centre,
                   double step, double at) {
  const double t = (at - first_centre) / step;
  if (t <= 0) {
    return values.first();
  }
  if (t >= values.size() - 1) {
    return values.last();
  }
  const int i = int(t);
  return values[i] + (values[i + 1] - values[i]) * (t - i);
}

/**
 * Shrink a walk range and a spread to what the field has room for: the cut of
 * the outermost pass may stray `walk + spread / 2` past the lattice edge, and
 * the field reaches only `margin` past it.
 */
void fitToMargin(double margin, double* walk, double* spread,
                 const char* axis) {
  const double asked_walk = *walk;
  const double asked_spread = *spread;
  margin = qMax(0.0, margin);
  if (*walk + *spread / 2 > margin + kEpsilon) {
    *walk = qMax(0.0, margin - *spread / 2);
  }
  if (*spread / 2 > margin + kEpsilon) {
    *spread = 2 * margin;
  }
  if (*walk != asked_walk || *spread != asked_spread) {
    qWarning() << "[Export] galvo K-pass seams:" << axis << "walk" << asked_walk
               << "mm and spread" << asked_spread << "mm do not fit the"
               << margin << "mm the field reaches past a tile -- using walk"
               << *walk << "mm, spread" << *spread << "mm";
  }
}

}  // namespace

std::shared_ptr<const GalvoKpassSeams> GalvoKpassSeams::build(
    const Params& params, const Lattice& lattice,
    const QVector<Image>& images) {
  auto seams = std::make_shared<GalvoKpassSeams>();
  GalvoKpassSeams& s = *seams;
  s.lattice_ = lattice;
  s.passes_ = qMax(1, params.passes);
  s.walk_length_ = params.walk_length > 0 ? params.walk_length : 10;

  // Directions, cycled over the passes. Anything but U and D is dropped.
  QVector<bool> letters;
  for (const QChar c : params.directions.toUpper()) {
    if (c == 'U' || c == 'D') {
      letters.append(c == 'U');
    } else {
      qWarning() << "[Export] galvo_pass_directions: ignoring" << c;
    }
  }
  if (letters.isEmpty()) {
    letters = {true, false};
  }
  for (int p = 0; p < s.passes_; p++) {
    s.up_.append(letters[p % letters.size()]);
  }

  const int vertical = lattice.max_col - lattice.min_col;
  const int horizontal = lattice.max_row - lattice.min_row;
  s.vwalk_ = qMax(0.0, params.vseam_walk);
  s.vspread_ = qMax(0.0, params.vseam_spread);
  s.hwalk_ = qMax(0.0, params.hseam_walk);
  s.hspread_ = qMax(0.0, params.hseam_spread);
  if (vertical > 0) {
    fitToMargin(lattice.half_field - lattice.step.width() / 2, &s.vwalk_,
                &s.vspread_, "vertical");
  }
  if (horizontal > 0) {
    fitToMargin(lattice.half_field - lattice.step.height() / 2, &s.hwalk_,
                &s.hspread_, "horizontal");
  }
  // Equally spaced, the first pass furthest to the right (and furthest down).
  for (int p = 0; p < s.passes_; p++) {
    const double t = s.passes_ > 1 ? double(p) / (s.passes_ - 1) : 0.5;
    s.voff_.append(s.vspread_ / 2 - t * s.vspread_);
    s.hoff_.append(s.hspread_ / 2 - t * s.hspread_);
  }
  // A pixel more than the geometry says, for the rounding of a cut onto sites.
  s.reach_x_ = vertical > 0
                   ? s.vwalk_ + s.vspread_ / 2 + 1 / lattice.px_per_mm_x
                   : 0;
  s.reach_y_ = horizontal > 0
                   ? s.hwalk_ + s.hspread_ / 2 + 1 / lattice.px_per_mm_y
                   : 0;

  QRect range;
  for (const Image& image : images) {
    range = range.united(image.rect);
  }
  s.col0_ = range.left();
  s.row0_ = range.top();
  s.cols_ = range.width();
  s.rows_ = range.height();
  s.min_cost_ = params.min_cost && s.vwalk_ > 0;

  const double pitch_x = 1 / lattice.px_per_mm_x;
  const double pitch_y = 1 / lattice.px_per_mm_y;
  for (int k = 0; k < vertical; k++) {
    Seam seam;
    seam.nominal = (lattice.min_col + k + 0.5) * lattice.step.width();
    seam.path = slowWalk(s.rows_, quint64(lattice.min_col + k + 1000), s.vwalk_,
                         pitch_y, s.walk_length_);
    seam.spread = QVector<float>(s.rows_, 1.0f);
    s.vseams_.append(seam);
  }
  for (int k = 0; k < horizontal; k++) {
    Seam seam;
    seam.nominal = (lattice.min_row + k + 0.5) * lattice.step.height();
    seam.path = slowWalk(s.cols_, quint64(lattice.min_row + k + 5000), s.hwalk_,
                         pitch_x, s.walk_length_);
    seam.spread = QVector<float>(s.cols_, 1.0f);
    s.hseams_.append(seam);
  }
  if (s.min_cost_) {
    for (int k = 0; k < vertical; k++) {
      s.minCostPath(k, images);
    }
  }

  // Who lays how much, and how far from their stop: a cell left with a handful
  // of dots hands them to the cell beside it rather than have the gantry stop
  // for them.
  struct Tally {
    int dots = 0;
    double min_x = 0, max_x = 0, sum_x = 0;
  };
  const int ncol = vertical + 1;
  const int nrow = horizontal + 1;
  QVector<Tally> tally(ncol * nrow);
  int inked_rows = 0;
  for (const Image& image : images) {
    if (!image.dots) {
      continue;
    }
    for (int y = 0; y < image.rect.height(); y++) {
      const uchar* line = image.dots->constScanLine(y);
      const int py = image.rect.top() + y;
      for (int x = 0; x < image.rect.width(); x++) {
        if (line[x] == 255) {
          continue;
        }
        const int px = image.rect.left() + x;
        int col, row;
        s.ownerUnmapped(px, py, &col, &row);
        Tally& t = tally[(row - lattice.min_row) * ncol + (col - lattice.min_col)];
        const double mm = (px + 0.5) * pitch_x - lattice.offset.x();
        if (t.dots == 0) {
          t.min_x = t.max_x = mm;
        } else {
          t.min_x = qMin(t.min_x, mm);
          t.max_x = qMax(t.max_x, mm);
        }
        t.sum_x += mm;
        t.dots++;
      }
    }
  }
  if (s.min_cost_) {
    for (int r = 0; r < nrow; r++) {
      for (int c = 0; c < ncol; c++) {
        const Tally& t = tally[r * ncol + c];
        if (t.dots == 0 || t.dots >= kFewDots) {
          continue;
        }
        const double mean = t.sum_x / t.dots;
        int order[2] = {c - 1, c + 1};
        auto centre = [&](int cc) {
          return (lattice.min_col + cc) * lattice.step.width();
        };
        if (std::abs(centre(order[1]) - mean) <
            std::abs(centre(order[0]) - mean)) {
          std::swap(order[0], order[1]);
        }
        for (int to : order) {
          if (to < 0 || to >= ncol) {
            continue;
          }
          const double reach = lattice.half_field - kHandoverMarginMm;
          if (std::abs(t.min_x - centre(to)) > reach ||
              std::abs(t.max_x - centre(to)) > reach) {
            continue;
          }
          s.handovers_.append({lattice.min_col + c, lattice.min_row + r,
                               lattice.min_col + to, t.dots});
          qInfo() << "[Export] galvo K-pass seams: tile"
                  << lattice.min_col + c << lattice.min_row + r << "has only"
                  << t.dots << "dots -- handed to tile" << lattice.min_col + to
                  << lattice.min_row + r;
          break;
        }
      }
    }
  }

  // How many rows cut right beside a dot, summed over the vertical seams: the
  // figure the cheapest path is there to bring down.
  for (int k = 0; k < s.vseams_.size(); k++) {
    for (int j = 0; j < s.rows_; j++) {
      const int py = s.row0_ + j;
      const double cut = s.verticalCut(k, py, s.passOf(py));
      const int i =
          int(std::floor((cut + lattice.offset.x()) * lattice.px_per_mm_x));
      // dots, not tone: a cut through a light area may still miss every dot
      bool inked = false;
      for (const Image& image : images) {
        if (!image.dots || py < image.rect.top() || py > image.rect.bottom()) {
          continue;
        }
        const uchar* line = image.dots->constScanLine(py - image.rect.top());
        for (int px = qMax(i - 1, image.rect.left());
             px <= qMin(i + 1, image.rect.right()); px++) {
          inked = inked || line[px - image.rect.left()] != 255;
        }
      }
      inked_rows += inked ? 1 : 0;
    }
  }
  s.inked_cut_rows_ = inked_rows;
  qInfo() << "[Export] galvo K-pass seams:" << s.passes_ << "passes,"
          << vertical << "vertical seam(s)"
          << (s.min_cost_ ? "on the cheapest path" : "on a walk") << "+/-"
          << s.vwalk_ << "mm, spread" << s.vspread_ << "mm;" << horizontal
          << "horizontal seam(s) +/-" << s.hwalk_ << "mm, spread" << s.hspread_
          << "mm; rows cut beside a dot:" << inked_rows;
  return seams;
}

int GalvoKpassSeams::countInked(const QVector<Image>& images, int row_px,
                                int from_px, int to_px) const {
  int count = 0;
  for (const Image& image : images) {
    if (!image.source || row_px < image.rect.top() ||
        row_px > image.rect.bottom()) {
      continue;
    }
    const uchar* line = image.source->constScanLine(row_px - image.rect.top());
    const int a = qMax(from_px, image.rect.left());
    const int b = qMin(to_px, image.rect.right());
    for (int px = a; px <= b; px++) {
      count += line[px - image.rect.left()] != 255 ? 1 : 0;
    }
  }
  return count;
}

void GalvoKpassSeams::minCostPath(int seam_index,
                                  const QVector<Image>& images) {
  Seam& seam = vseams_[seam_index];
  const double ppx = lattice_.px_per_mm_x;
  const double ppy = lattice_.px_per_mm_y;
  // Cells of whole pixels, as near the nominal size as the grid allows.
  const int cell_x = qMax(1, qRound(kCostCellMm * ppx));
  const int cell_y = qMax(1, qRound(kCostCellMm * ppy));
  const double cw = cell_x / ppx;  // mm
  const double ch = cell_y / ppy;
  const int hc = rows_ / cell_y;
  if (hc < 2) {
    return;
  }
  const double half = vspread_ / 2;
  const int nb = qRound(vwalk_ / cw);  // cells the path may stray, each side
  const int nh = qRound(half / cw);    // cells the offsets reach, each side
  if (nb < 1) {
    return;
  }
  const int n = 2 * nb + 1;            // candidate positions
  const int ncol = 2 * (nb + nh) + 1;  // cells of tone needed around them
  // The pixel column whose left edge is the nominal line.
  const int i0 = qRound((seam.nominal + lattice_.offset.x()) * ppx);
  const int lo = i0 - (nb + nh) * cell_x - cell_x / 2;

  // Mean tone and "any ink at all", per cell.
  QVector<float> coarse(hc * ncol, 0.0f);
  QVector<uchar> inked(hc * ncol, 0);
  for (const Image& image : images) {
    if (!image.source) {
      continue;
    }
    const int x_from = qMax(lo, image.rect.left());
    const int x_to = qMin(lo + ncol * cell_x - 1, image.rect.right());
    const int y_from = qMax(row0_, image.rect.top());
    const int y_to = qMin(row0_ + hc * cell_y - 1, image.rect.bottom());
    for (int py = y_from; py <= y_to; py++) {
      const uchar* line = image.source->constScanLine(py - image.rect.top());
      const int r = (py - row0_) / cell_y;
      for (int px = x_from; px <= x_to; px++) {
        const uchar v = line[px - image.rect.left()];
        if (v == 255) {
          continue;
        }
        const int c = (px - lo) / cell_x;
        coarse[r * ncol + c] += float(1 - v / 255.0);
        inked[r * ncol + c] = 1;
      }
    }
  }
  const float cell_area = float(cell_x * cell_y);
  for (float& v : coarse) {
    v /= cell_area;
  }

  // The walk, at the middle of each row of cells: what the path is pulled to.
  QVector<double> walk(hc);
  for (int r = 0; r < hc; r++) {
    walk[r] = seam.path[qMin(rows_ - 1, r * cell_y + cell_y / 2)];
  }
  // Cost of standing at candidate k in row r: the tone averaged over the band
  // the K offsets cover around it, plus the pull. Nothing if the candidate sits
  // in a blank strip wide enough to take all K cuts once they close up.
  const int strip = qMax(0, int(std::ceil((kBlankStripMm / cw - 1) / 2)));
  QVector<float> cost(hc * n);
  for (int r = 0; r < hc; r++) {
    const float* tone = coarse.constData() + r * ncol;
    const uchar* ink = inked.constData() + r * ncol;
    QVector<double> tone_sum(ncol + 1, 0.0);
    QVector<int> ink_sum(ncol + 1, 0);
    for (int c = 0; c < ncol; c++) {
      tone_sum[c + 1] = tone_sum[c] + tone[c];
      ink_sum[c + 1] = ink_sum[c] + ink[c];
    }
    for (int k = 0; k < n; k++) {
      const int centre = nh + k;  // this candidate's own cell
      double band =
          (tone_sum[k + 2 * nh + 1] - tone_sum[k]) / (2 * nh + 1);
      const int a = qMax(0, centre - strip);
      const int b = qMin(ncol - 1, centre + strip);
      if (centre - strip >= 0 && centre + strip < ncol &&
          ink_sum[b + 1] - ink_sum[a] == 0) {
        band = 0;
      }
      const double offset = (k - nb) * cw;
      cost[r * n + k] =
          float(band + kCostPull * std::abs(offset - walk[r]) /
                           qMax(vwalk_, kEpsilon));
    }
  }

  // Seam carving: down the rows, at most one cell sideways per row.
  QVector<float> acc(n), next(n);
  QVector<qint8> back(hc * n, 0);
  for (int k = 0; k < n; k++) {
    acc[k] = cost[k];
  }
  for (int r = 1; r < hc; r++) {
    for (int k = 0; k < n; k++) {
      float best = acc[k];
      qint8 from = 0;
      if (k > 0 && acc[k - 1] + kCostTurn < best) {
        best = acc[k - 1] + float(kCostTurn);
        from = -1;
      }
      if (k + 1 < n && acc[k + 1] + kCostTurn < best) {
        best = acc[k + 1] + float(kCostTurn);
        from = 1;
      }
      back[r * n + k] = from;
      next[k] = best + cost[r * n + k];
    }
    std::swap(acc, next);
  }
  QVector<int> path(hc);
  path[hc - 1] = int(std::min_element(acc.begin(), acc.end()) - acc.begin());
  for (int r = hc - 1; r > 0; r--) {
    path[r - 1] = path[r] + back[r * n + path[r]];
  }

  // The raw path, and a moving average of it over kPathSmoothMm.
  QVector<double> raw(hc), smooth(hc);
  for (int r = 0; r < hc; r++) {
    raw[r] = (path[r] - nb) * cw;
  }
  const int window = qMax(1, qRound(kPathSmoothMm / ch)) | 1;
  for (int r = 0; r < hc; r++) {
    double sum = 0;
    for (int d = -window / 2; d <= window / 2; d++) {
      sum += raw[qBound(0, r + d, hc - 1)];
    }
    smooth[r] = sum / window;
  }

  // Back onto the pixel rows.
  const int half_strip_px = int(std::ceil(kBlankStripMm / 2 * ppx));
  const int half_spread_px = int(std::ceil(half * ppx)) + 1;
  auto cut_px = [&](double offset) {
    return qRound((seam.nominal + offset + lattice_.offset.x()) * ppx);
  };
  QVector<uchar> in_strip(rows_, 0), needs(rows_, 0);
  for (int j = 0; j < rows_; j++) {
    const int py = row0_ + j;
    double at = qBound(-vwalk_,
                       interpolate(smooth, cell_y / 2.0, cell_y, j), vwalk_);
    const int i = cut_px(at);
    if (countInked(images, py, i - half_strip_px, i + half_strip_px) > 0) {
      // Smoothing has pushed the path out of a strip too narrow to be
      // smoothed in; the unsmoothed position is still inside it.
      const double unsmoothed = interpolate(raw, cell_y / 2.0, cell_y, j);
      const int iu = cut_px(unsmoothed);
      if (countInked(images, py, iu - half_strip_px, iu + half_strip_px) == 0) {
        at = unsmoothed;
      }
    }
    seam.path[j] = float(at);
    const int ia = cut_px(at);
    in_strip[j] =
        countInked(images, py, ia - half_strip_px, ia + half_strip_px) == 0;
    needs[j] =
        countInked(images, py, ia - half_spread_px, ia + half_spread_px) > 0;
  }

  // Close the offsets up along every stretch that runs in a blank strip and
  // would otherwise put a cut on ink. Rows with no ink anywhere near may lie
  // inside such a stretch; it starts and ends on rows that need it.
  QVector<uchar> closed(rows_, 0);
  const int min_run = int(std::ceil(kSpreadRunMm * ppy));
  for (int j = 0; j < rows_;) {
    if (!(in_strip[j] && needs[j])) {
      j++;
      continue;
    }
    int end = j;
    int k = j;
    while (k < rows_ && in_strip[k]) {
      if (needs[k]) {
        end = k;
      }
      k++;
    }
    if (end - j + 1 >= min_run) {
      for (int q = j; q <= end; q++) {
        closed[q] = 1;
      }
    }
    j = k;
  }
  // ... and open them again over kSpreadRampMm either side of it.
  const double ramp_rows = kSpreadRampMm * ppy;
  int nearest = -1;
  QVector<int> distance(rows_, std::numeric_limits<int>::max());
  for (int j = 0; j < rows_; j++) {
    if (closed[j]) {
      nearest = j;
    }
    if (nearest >= 0) {
      distance[j] = j - nearest;
    }
  }
  nearest = -1;
  for (int j = rows_ - 1; j >= 0; j--) {
    if (closed[j]) {
      nearest = j;
    }
    if (nearest >= 0) {
      distance[j] = qMin(distance[j], nearest - j);
    }
  }
  for (int j = 0; j < rows_; j++) {
    seam.spread[j] = distance[j] == std::numeric_limits<int>::max()
                         ? 1.0f
                         : float(qMin(1.0, distance[j] / ramp_rows));
  }
}

double GalvoKpassSeams::verticalCut(int seam, int row_px, int pass) const {
  const Seam& s = vseams_[seam];
  const int j = row_px - row0_;
  if (j < 0 || j >= rows_) {
    return s.nominal + voff_[pass];
  }
  return s.nominal + s.path[j] + voff_[pass] * s.spread[j];
}

double GalvoKpassSeams::horizontalCut(int seam, int col_px, int pass) const {
  const Seam& s = hseams_[seam];
  const int i = col_px - col0_;
  if (i < 0 || i >= cols_) {
    return s.nominal + hoff_[pass];
  }
  return s.nominal + s.path[i] + hoff_[pass] * s.spread[i];
}

void GalvoKpassSeams::ownerUnmapped(int px, int py, int* col, int* row) const {
  const double x = (px + 0.5) / lattice_.px_per_mm_x - lattice_.offset.x();
  const double y = (py + 0.5) / lattice_.px_per_mm_y - lattice_.offset.y();
  const int pass = passOf(py);
  int c = lattice_.min_col;
  for (int k = 0; k < vseams_.size(); k++) {
    if (x >= verticalCut(k, py, pass)) {
      c++;
    }
  }
  int r = lattice_.min_row;
  for (int k = 0; k < hseams_.size(); k++) {
    if (y >= horizontalCut(k, px, pass) - kEpsilon) {
      r++;
    }
  }
  *col = c;
  *row = r;
}

void GalvoKpassSeams::owner(int px, int py, int* col, int* row) const {
  ownerUnmapped(px, py, col, row);
  for (const Handover& h : handovers_) {
    if (h.col == *col && h.row == *row) {
      *col = h.to_col;
      return;
    }
  }
}
