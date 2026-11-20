#pragma once

#include <gtest/gtest.h>
#include <toolpath_exporter/factories/galvo-kpass-seams.h>

#include <QImage>
#include <algorithm>
#include <cmath>
#include <functional>

namespace galvo_kpass_test {

constexpr double kPpm = 20;  // 0.05 mm pixels, as the seam tests ran

/** A fixed pseudo-random threshold per site, standing in for a dither. */
inline quint32 siteHash(quint32 x, quint32 y) {
  quint32 h = x * 374761393u + y * 668265263u;
  h = (h ^ (h >> 13)) * 1274126177u;
  return h ^ (h >> 16);
}

struct Picture {
  QRect rect;
  QImage source;
  QImage dots;
  QVector<GalvoKpassSeams::Image> images() const {
    return {{rect, &dots, &source}};
  }
};

/** `tone(x mm, y mm)` in 0 (blank) .. 1, sampled over `mm`. */
inline Picture makePicture(const QRectF& mm,
                           const std::function<double(double, double)>& tone) {
  Picture p;
  p.rect = QRect(qRound(mm.left() * kPpm), qRound(mm.top() * kPpm),
                 qRound(mm.width() * kPpm), qRound(mm.height() * kPpm));
  p.source = QImage(p.rect.size(), QImage::Format_Grayscale8);
  p.dots = QImage(p.rect.size(), QImage::Format_Grayscale8);
  for (int y = 0; y < p.rect.height(); y++) {
    uchar* s = p.source.scanLine(y);
    uchar* d = p.dots.scanLine(y);
    for (int x = 0; x < p.rect.width(); x++) {
      const int px = p.rect.left() + x;
      const int py = p.rect.top() + y;
      const double t = tone((px + 0.5) / kPpm, (py + 0.5) / kPpm);
      s[x] = uchar(qRound(255 * (1 - t)));
      d[x] = t > (siteHash(px, py) % 10000 + 0.5) / 10000.0 ? 0 : 255;
    }
  }
  return p;
}

/** 3 x 2 tiles of 90 mm in a 110 mm field: seams at x 315, 405 and y 225. */
inline GalvoKpassSeams::Lattice lattice90() {
  GalvoKpassSeams::Lattice l;
  l.step = QSizeF(90, 90);
  l.min_col = 3;
  l.max_col = 5;
  l.min_row = 2;
  l.max_row = 3;
  l.half_field = 55;
  l.px_per_mm_x = l.px_per_mm_y = kPpm;
  return l;
}

/** 40 % tone with a 3 mm blank strip 3 to 6 mm right of the first seam. */
inline Picture stripPicture() {
  return makePicture(QRectF(230, 140, 270, 170), [](double x, double y) {
    return x >= 318 && x < 321 && y >= 170 && y < 290 ? 0.0 : 0.4;
  });
}

}  // namespace galvo_kpass_test

TEST(GalvoKpassSeamsTest, EveryDotHasOneOwnerWithinItsField) {
  using namespace galvo_kpass_test;
  const Picture pic = stripPicture();
  const auto lattice = lattice90();
  const auto seams = GalvoKpassSeams::build({}, lattice, pic.images());
  const auto again = GalvoKpassSeams::build({}, lattice, pic.images());
  ASSERT_EQ(seams->verticalSeams(), 2);
  ASSERT_EQ(seams->horizontalSeams(), 1);
  long out_of_lattice = 0, out_of_field = 0, differ = 0;
  long cells[2][3] = {};
  for (int y = 0; y < pic.rect.height(); y++) {
    const uchar* line = pic.dots.constScanLine(y);
    for (int x = 0; x < pic.rect.width(); x++) {
      if (line[x] == 255) continue;
      const int px = pic.rect.left() + x, py = pic.rect.top() + y;
      int c, r, c2, r2;
      seams->owner(px, py, &c, &r);
      again->owner(px, py, &c2, &r2);
      differ += c != c2 || r != r2;
      if (c < 3 || c > 5 || r < 2 || r > 3) {
        out_of_lattice++;
        continue;
      }
      cells[r - 2][c - 3]++;
      out_of_field += std::abs((px + 0.5) / kPpm - c * 90.0) > 55 ||
                      std::abs((py + 0.5) / kPpm - r * 90.0) > 55;
    }
  }
  EXPECT_EQ(out_of_lattice, 0);
  EXPECT_EQ(out_of_field, 0);
  EXPECT_EQ(differ, 0);  // the same document twice gives the same seams
  for (auto& row : cells) {
    for (long n : row) EXPECT_GT(n, 0);
  }
}

TEST(GalvoKpassSeamsTest, PassesAndDirections) {
  using namespace galvo_kpass_test;
  const Picture pic = stripPicture();
  const auto seams = GalvoKpassSeams::build({}, lattice90(), pic.images());
  EXPECT_EQ(seams->passes(), 4);
  EXPECT_TRUE(seams->passUp(0));
  EXPECT_FALSE(seams->passUp(1));
  EXPECT_TRUE(seams->passUp(2));
  EXPECT_FALSE(seams->passUp(3));
  EXPECT_EQ(seams->passOf(9), 1);
  EXPECT_EQ(seams->passOf(-1), 3);

  GalvoKpassSeams::Params three;
  three.passes = 3;
  three.directions = "ud?";  // cycled; the unknown letter is dropped
  const auto k3 = GalvoKpassSeams::build(three, lattice90(), pic.images());
  EXPECT_EQ(k3->passes(), 3);
  EXPECT_TRUE(k3->passUp(0));
  EXPECT_FALSE(k3->passUp(1));
  EXPECT_TRUE(k3->passUp(2));

  GalvoKpassSeams::Params one;
  one.passes = 1;
  one.directions = "";  // empty alternates from U
  const auto k1 = GalvoKpassSeams::build(one, lattice90(), pic.images());
  EXPECT_EQ(k1->passes(), 1);
  EXPECT_TRUE(k1->passUp(0));
}

TEST(GalvoKpassSeamsTest, WalkSpreadsThePassesEvenly) {
  using namespace galvo_kpass_test;
  const Picture pic = stripPicture();
  GalvoKpassSeams::Params params;
  params.min_cost = false;
  const auto seams = GalvoKpassSeams::build(params, lattice90(), pic.images());
  const int row = pic.rect.top() + 1000, col = pic.rect.left() + 1000;
  // first pass furthest right (and furthest down), the rest equally spaced
  for (int p = 0; p < 3; p++) {
    EXPECT_NEAR(seams->verticalCut(0, row, p) - seams->verticalCut(0, row, p + 1), 2.5, 1e-5);
    EXPECT_NEAR(seams->horizontalCut(0, col, p) - seams->horizontalCut(0, col, p + 1), 2.0 / 3, 1e-5);
  }
  for (int py = pic.rect.top(); py <= pic.rect.bottom(); py++) {
    const double mid = (seams->verticalCut(1, py, 0) + seams->verticalCut(1, py, 3)) / 2;
    ASSERT_LE(std::abs(mid - 405), 6 + 1e-6);
  }
}

TEST(GalvoKpassSeamsTest, CheapestPathFindsABlankStripAndClosesTheOffsets) {
  using namespace galvo_kpass_test;
  const Picture pic = stripPicture();
  GalvoKpassSeams::Params walk_params;
  walk_params.min_cost = false;
  const auto walk = GalvoKpassSeams::build(walk_params, lattice90(), pic.images());
  const auto seams = GalvoKpassSeams::build({}, lattice90(), pic.images());
  EXPECT_TRUE(seams->minCost());
  EXPECT_LT(seams->inkedCutRows(), walk->inkedCutRows());
  int rows = 0, inside = 0, closed = 0;
  for (int py = qRound(175 * kPpm); py < qRound(285 * kPpm); py++) {
    double lo = 1e9, hi = -1e9;
    for (int p = 0; p < 4; p++) {
      const double cut = seams->verticalCut(0, py, p);
      lo = std::min(lo, cut);
      hi = std::max(hi, cut);
    }
    rows++;
    inside += lo >= 318 && hi <= 321;
    closed += hi - lo < 1e-6;
  }
  EXPECT_GT(inside, rows * 0.97);  // all four cuts fall in the blank
  EXPECT_GT(closed, rows * 0.97);  // on one line
  for (int py = pic.rect.top(); py <= pic.rect.bottom(); py++) {
    const double mid = (seams->verticalCut(0, py, 0) + seams->verticalCut(0, py, 3)) / 2;
    ASSERT_LE(std::abs(mid - 315), 6 + 1e-6);  // never further than the walk may go
  }
}

TEST(GalvoKpassSeamsTest, ShrinksToWhatTheFieldReaches) {
  using namespace galvo_kpass_test;
  const Picture pic = stripPicture();
  // tile 100 in a 110 field leaves 5 mm: the walk gives way first
  GalvoKpassSeams::Lattice tight = lattice90();
  tight.step = QSizeF(100, 100);
  tight.min_col = 2;
  tight.max_col = 4;
  tight.min_row = 1;
  tight.max_row = 2;
  const auto shrunk = GalvoKpassSeams::build({}, tight, pic.images());
  EXPECT_NEAR(shrunk->usedVWalk(), 1.25, 1e-9);
  EXPECT_NEAR(shrunk->usedVSpread(), 7.5, 1e-9);
  EXPECT_NEAR(shrunk->usedHWalk(), 1, 1e-9);
  EXPECT_NEAR(shrunk->usedHSpread(), 2, 1e-9);
  // no margin at all: straight cuts on the lattice edge
  GalvoKpassSeams::Lattice none = tight;
  none.step = QSizeF(110, 110);
  const auto zero = GalvoKpassSeams::build({}, none, pic.images());
  EXPECT_EQ(zero->usedVWalk(), 0);
  EXPECT_EQ(zero->usedVSpread(), 0);
  EXPECT_EQ(zero->usedHWalk(), 0);
  EXPECT_EQ(zero->usedHSpread(), 0);
  EXPECT_FALSE(zero->minCost());
}

TEST(GalvoKpassSeamsTest, OneTileHasNoSeams) {
  using namespace galvo_kpass_test;
  const Picture pic = stripPicture();
  GalvoKpassSeams::Lattice single = lattice90();
  single.min_col = single.max_col = 4;
  single.min_row = single.max_row = 2;
  const auto seams = GalvoKpassSeams::build({}, single, pic.images());
  EXPECT_EQ(seams->verticalSeams(), 0);
  EXPECT_EQ(seams->horizontalSeams(), 0);
  EXPECT_EQ(seams->reachX(), 0);
  EXPECT_EQ(seams->passes(), 4);  // the passes are still made
  int c = 0, r = 0;
  seams->owner(pic.rect.left() + 5, pic.rect.top() + 5, &c, &r);
  EXPECT_EQ(c, 4);
  EXPECT_EQ(r, 2);
}

TEST(GalvoKpassSeamsTest, ATileLeftWithAFewDotsHandsThemOver) {
  using namespace galvo_kpass_test;
  // a solid square in the left tile and a 0.6 mm speck 7 mm into the right one
  const Picture pic = makePicture(QRectF(230, 140, 100, 80), [](double x, double y) {
    return (x >= 240 && x < 300 && y >= 150 && y < 210) ||
                   (x >= 322.0 && x < 322.6 && y >= 180 && y < 180.6)
               ? 1.0
               : 0.0;
  });
  GalvoKpassSeams::Lattice two = lattice90();
  two.max_col = 4;
  two.max_row = 2;
  GalvoKpassSeams::Params params;
  params.vseam_walk = 0.6;  // too little to go round the speck
  params.vseam_spread = 0;
  const auto seams = GalvoKpassSeams::build(params, two, pic.images());
  ASSERT_EQ(seams->handovers().size(), 1);
  EXPECT_EQ(seams->handovers()[0].col, 4);
  EXPECT_EQ(seams->handovers()[0].to_col, 3);
  int c = 0, r = 0;
  seams->owner(qRound(322.3 * kPpm), qRound(180.3 * kPpm), &c, &r);
  EXPECT_EQ(c, 3);
  // without the cheapest path nothing is handed over
  params.min_cost = false;
  EXPECT_EQ(GalvoKpassSeams::build(params, two, pic.images())->handovers().size(), 0);
}
