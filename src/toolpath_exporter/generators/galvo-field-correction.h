#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QPointF>
#include <QtGlobal>
#include <cmath>
#include <vector>

/**
 * Pre-distortion of galvo field coordinates (beam-studio
 * docs/prd/hexa-ii-galvo/galvo-grid-correction.md).
 *
 * The card's own grid correction turned out not to be usable (2026-11-04: a
 * loaded table drops the eight manual coefficients, and a non-linear one comes
 * out partly applied and locally folded), so the residual the manual
 * coefficients leave -- mostly the f-theta cubic, 0.3-0.5 mm -- is taken out
 * here instead, before the coordinates reach the card.
 *
 * The table is the field's displacement D: where a point commanded at c lands,
 * minus c, mm, on a size x size grid over +-field/2, row-major from
 * (-field/2, -field/2) with X fastest -- what the coupon solver measures with
 * the manual coefficients in force. To land at c the beam is commanded at the
 * c' that solves c' + D(c') = c, found by fixed-point iteration; D's gradient
 * is around 1%, so three rounds leave an error far below a micron.
 *
 * D is interpolated bilinearly inside the grid and extrapolated from the edge
 * cells outside it.
 */
class GalvoFieldCorrection {
 public:
  static GalvoFieldCorrection fromJson(const QJsonObject& obj) {
    GalvoFieldCorrection c;
    const int size = obj.value("size").toInt(0);
    const double field = obj.value("field").toDouble(0);
    const QJsonArray dx = obj.value("dx").toArray();
    const QJsonArray dy = obj.value("dy").toArray();
    if (size < 2 || field <= 0 || dx.size() != size * size || dy.size() != size * size) {
      return c;  // not enabled
    }
    c.size_ = size;
    c.field_ = field;
    c.dx_.reserve(size * size);
    c.dy_.reserve(size * size);
    for (int i = 0; i < size * size; i++) {
      c.dx_.push_back(dx[i].toDouble());
      c.dy_.push_back(dy[i].toDouble());
    }
    return c;
  }

  bool enabled() const { return size_ >= 2; }

  /** D at a field-local point, mm. */
  QPointF displacement(const QPointF& p) const {
    if (!enabled()) return QPointF();
    const double step = field_ / (size_ - 1);
    const double fx = (p.x() + field_ / 2) / step;
    const double fy = (p.y() + field_ / 2) / step;
    // the cell, clamped to the grid; outside it the edge cell's plane carries on
    const int i = qBound(0, int(std::floor(fx)), size_ - 2);
    const int j = qBound(0, int(std::floor(fy)), size_ - 2);
    const double u = fx - i;
    const double v = fy - j;
    auto at = [this](const std::vector<double>& d, int col, int row) {
      return d[size_t(row) * size_t(size_) + size_t(col)];
    };
    auto lerp = [&](const std::vector<double>& d) {
      return (1 - u) * (1 - v) * at(d, i, j) + u * (1 - v) * at(d, i + 1, j) +
             (1 - u) * v * at(d, i, j + 1) + u * v * at(d, i + 1, j + 1);
    };
    return QPointF(lerp(dx_), lerp(dy_));
  }

  /** Where to command the beam for it to land at `target`. */
  QPointF predistort(const QPointF& target) const {
    if (!enabled()) return target;
    QPointF q = target;
    for (int k = 0; k < 3; k++) {
      q = target - displacement(q);
    }
    return q;
  }

 private:
  int size_ = 0;
  double field_ = 0;
  std::vector<double> dx_;
  std::vector<double> dy_;
};
