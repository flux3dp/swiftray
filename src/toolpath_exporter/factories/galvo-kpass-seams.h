#pragma once

#include <QImage>
#include <QPointF>
#include <QRect>
#include <QSizeF>
#include <QString>
#include <QVector>
#include <memory>

/**
 * The seams of the K-pass walk strategy ("dot-kpass-walk") for one dithered
 * raster: which lattice cell lays each dot.
 *
 * A dithered image is laid in K passes per stop, pass p taking the rows whose
 * index on the machine's pixel grid is p modulo K. A seam is not a straight
 * line and not a band: each vertical seam is a path wandering either side of
 * the lattice edge, and every pass cuts at that path plus an offset of its own,
 * so the K cuts of one seam lie spread across a few millimetres and no two
 * neighbouring rows change hands at the same place. Horizontal seams work the
 * same way along x. A dot belongs to the cell its position falls in once every
 * seam has been cut for its row's pass -- exactly one cell, so nothing is laid
 * twice and nothing is left out.
 *
 * The path of a vertical seam is either a slow random walk, or (min_cost) the
 * cheapest path within the walk's range through the image's own tone: it
 * drifts into blank areas and around small features that poke across the
 * lattice edge, and is the walk wherever the image gives it no reason to
 * leave. Where such a path runs in a blank strip the per-pass offsets close
 * up, so all K cuts fall inside the blank.
 *
 * Everything is worked out once for the whole lattice and then only read, so
 * every block asks the same object and gets answers that agree.
 * Measured on HEXA G: playground test61 (f, g, i), see
 * beam-studio docs/prd/hexa-ii-galvo/galvo-seam-kpass-walk.md section 7.4.
 */
class GalvoKpassSeams {
 public:
  struct Params {
    int passes = 4;
    /** One letter per pass, cycled: U = bottom to top, D = top to bottom. */
    QString directions = "UDUD";
    /** mm either side of the lattice edge the path may wander. */
    double vseam_walk = 6;
    /** mm from the first pass's cut to the last one's. */
    double vseam_spread = 7.5;
    double hseam_walk = 1;
    double hseam_spread = 2;
    /** mm of seam over which the walk turns once. */
    double walk_length = 10;
    /** Vertical seams follow the cheapest path through the tone. */
    bool min_cost = true;
  };

  /** The lattice of stops, and the pixel grid the dots sit on. */
  struct Lattice {
    /** mm between stops; a cell is centred on (col * width, row * height). */
    QSizeF step;
    int min_col = 0;
    int max_col = 0;
    int min_row = 0;
    int max_row = 0;
    /** mm from a stop to the edge of what its beam reaches. */
    double half_field = 55;
    double px_per_mm_x = 20;
    double px_per_mm_y = 20;
    /** mm: a pixel centre is at (px + 0.5) / px_per_mm - offset. */
    QPointF offset;
  };

  /**
   * One dithered image. `rect` places it on the pixel grid; `dots` is
   * Grayscale8 with a dot wherever a pixel is not 255; `source` is the tone it
   * was dithered from, Grayscale8, 255 being blank. `source` may be null, and
   * the image is then taken to be blank for the purposes of the path.
   */
  struct Image {
    QRect rect;
    const QImage* dots = nullptr;
    const QImage* source = nullptr;
  };

  static std::shared_ptr<const GalvoKpassSeams> build(
      const Params& params, const Lattice& lattice,
      const QVector<Image>& images);

  int passes() const { return passes_; }
  /** Which pass lays the row with this index on the pixel grid. */
  int passOf(int row_px) const {
    return ((row_px % passes_) + passes_) % passes_;
  }
  /** Whether a pass runs its rows bottom to top. */
  bool passUp(int pass) const { return up_[pass]; }

  /** The lattice cell that lays the dot at this pixel. */
  void owner(int px, int py, int* col, int* row) const;

  /**
   * How far past its own edges a cell may be handed dots, mm. Zero along an
   * axis the lattice does not split.
   */
  double reachX() const { return reach_x_; }
  double reachY() const { return reach_y_; }

  // --- for the debug image and the tests ----------------------------------
  int verticalSeams() const { return vseams_.size(); }
  int horizontalSeams() const { return hseams_.size(); }
  /** mm, in the frame dots are emitted in. */
  double verticalCut(int seam, int row_px, int pass) const;
  double horizontalCut(int seam, int col_px, int pass) const;
  /** The walk range and spread after the field's margin has had its say. */
  double usedVWalk() const { return vwalk_; }
  double usedVSpread() const { return vspread_; }
  double usedHWalk() const { return hwalk_; }
  double usedHSpread() const { return hspread_; }
  bool minCost() const { return min_cost_; }
  /** Rows of all vertical seams whose cut for their pass has a dot beside it. */
  int inkedCutRows() const { return inked_cut_rows_; }
  /** Cells whose few dots were handed to a neighbour: (col, row) -> col. */
  struct Handover {
    int col;
    int row;
    int to_col;
    int dots;
  };
  const QVector<Handover>& handovers() const { return handovers_; }
  QRect pixelRange() const { return QRect(col0_, row0_, cols_, rows_); }
  const Lattice& lattice() const { return lattice_; }

 private:
  struct Seam {
    double nominal = 0;      // mm
    QVector<float> path;     // mm off the nominal line, per row / column
    QVector<float> spread;   // 0..1 of the pass offset, per row / column
  };
  void minCostPath(int seam_index, const QVector<Image>& images);
  int countInked(const QVector<Image>& images, int row_px, int from_px,
                 int to_px) const;
  void ownerUnmapped(int px, int py, int* col, int* row) const;

  Lattice lattice_;
  int passes_ = 1;
  QVector<bool> up_;
  double vwalk_ = 0, vspread_ = 0, hwalk_ = 0, hspread_ = 0;
  double walk_length_ = 10;
  bool min_cost_ = false;
  double reach_x_ = 0, reach_y_ = 0;
  QVector<double> voff_;  // mm per pass
  QVector<double> hoff_;
  int col0_ = 0, row0_ = 0, cols_ = 0, rows_ = 0;
  QVector<Seam> vseams_;  // indexed by row - row0_
  QVector<Seam> hseams_;  // indexed by col - col0_
  QVector<Handover> handovers_;
  int inked_cut_rows_ = 0;
};
