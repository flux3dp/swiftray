#pragma once

#include <QImage>
#include <QRectF>
#include <QVector>

#include "base-factory.h"

/**
 * Raster for a galvo head, cut as vectors.
 *
 * The fcode raster commands expand a bitmap on the machine, and those are not
 * allowed down the galvo path until phase 2 (HX2_GALVO_PROTOCOL.md §19.5). So
 * the bitmap is expanded here instead and leaves as ordinary galvo geometry,
 * which is what the protocol has in mind: nothing raster-shaped reaches the
 * board, only marks and dots.
 *
 * Two shapes come out of it, matching what the gcode path does for the same
 * images:
 *
 *  - a run of dark pixels becomes one marked segment, for a binarised image or
 *    one pass of a depth engraving;
 *  - a dithered image becomes one dot per dark pixel, the tone carried by how
 *    densely the dither placed them. The writer turns those into LASER_ON when
 *    a dotting time is set, so nothing here needs to know about the opcode.
 *
 * Splitting is not this class's business: moves leave through `proc`, so the
 * block lattice and cell clip apply to them without knowing they exist.
 */
class LaserRasterGalvoFactory : public BaseFactory {
 public:
  /** How the density falls off across an overlap band. */
  enum class BlendProfile { Simple, Granular, SuperGranular };

  /**
   * What a block needs to fade its dots out towards a seam.
   *
   * Blocks that share a seam all sample a band straddling it, and each lays
   * only some of the dots there, so together they come to one full covering.
   * Only a dithered image can take this: its tone already lives in how densely
   * the dots sit, so thinning them reads as a fade rather than a gap.
   *
   * Who lays which dot is settled by weight, not by agreement. Every block that
   * samples a site works out the weight of *all* the blocks that sample it,
   * normalises them so they sum to one, and walks that distribution against a
   * target drawn from an ordered screen. Exactly one block finds itself the
   * owner, and it needs no coordination to know it -- the others compute the
   * same answer and stay quiet. That is what keeps a corner, where four blocks
   * meet, from being burned twice.
   */
  struct Blend {
    bool active = false;
    /** Lattice step, mm: also the size of one nominal cell. */
    QSizeF step;
    /** Which cell this block is, and how far the lattice runs. */
    int col = 0;
    int row = 0;
    int min_col = 0;
    int max_col = 0;
    int min_row = 0;
    int max_row = 0;
    /** Total band width, mm; half of it lies each side of a shared edge. */
    double overlap = 10;
    BlendProfile profile = BlendProfile::Granular;
    /**
     * mm. How far along a scan line ownership holds before it can change hands,
     * for the runs a binarised or depth image is made of. A dithered image
     * changes owner site by site, which is what makes it fade; a run cannot --
     * alternating per pixel would shred it into single-pixel marks -- so its
     * ownership is decided in stretches this long instead, and the seam becomes
     * an interlocking edge rather than a straight one.
     *
     * A run carries no tone to fade, so the teeth are here to hide a seam the
     * gantry did not land squarely, not to blend anything, and each one costs a
     * jump. On a solid fill crossing every seam, teeth this long cost 17% of the
     * run time; at 2 mm they cost 39%. Nothing is lost either way -- the marked
     * length is identical to a hard seam -- so this trades time for how visible
     * a misplaced seam is.
     */
    double segment_length = 10;
  };

  explicit LaserRasterGalvoFactory(const FactoryKwargs& kwargs) noexcept;
  void set_blend(const Blend& blend) { blend_ = blend; }
  /** The region a block samples: its cell, grown into every shared seam. */
  QRectF blend_band() const;

  /**
   * `gray` is 8-bit greyscale already scaled to this factory's pixels, placed
   * with its top-left at `bbox_px.topLeft()`. `dots` picks one dot per pixel
   * over one segment per run.
   */
  void add_bitmap(const QImage& gray, const QRectF& bbox_px, bool dots);
  bool is_empty() const { return bitmaps_.isEmpty(); }
  int get_size() const { return bitmaps_.size(); }
  void clear() { bitmaps_.clear(); }
  /** Dwell for a dot, microseconds; the layer's dotting time. */
  void set_dotting_time(double us) { dotting_time_us_ = us; }

  /** Bounds in the frame the moves are emitted in (mm, offset applied). */
  QRectF get_bounds_mm() const;

  /**
   * Emit one pass. A pixel darker than `threshold` is engraved, so 255 takes
   * everything that is not pure white -- which is what a binarised or dithered
   * image wants -- and a depth pass walks the threshold down instead.
   *
   * `transposed` scans down the columns rather than along the rows. Depth
   * engraving alternates it pass to pass so the passes cross each other.
   */
  void generate_task_code(float speed, int threshold = 255,
                          bool transposed = false);

 private:
  struct Entry {
    QImage gray;
    QRectF bbox_px;
    bool dots = false;
  };

  /** One scan line: a row of the image, or a column when transposed. */
  void emitLine(const Entry& entry, int index, int threshold, bool reversed,
                bool transposed, float speed);
  void setPwm(float pwm);
  double profileDensity(double progress) const;
  /** The nominal cell of a lattice position. */
  QRectF cellAt(int col, int row) const;
  /** A cell grown into every seam it shares -- what that block samples. */
  QRectF bandAt(int col, int row) const;
  /** How much of this site belongs to that cell, before normalising. */
  double rawWeight(int col, int row, double x, double y) const;
  /** Whether this block, of all that sample it, is the one that lays this site. */
  bool ownsSite(double x, double y, double target) const;
  /** Ordered-screen target: a dithered image changes owner site by site. */
  double siteTarget(int column, int row) const;
  /** Coarse target: a run holds its owner for a stretch of the scan line. */
  double runTarget(double x, double y) const;

  Blend blend_;
  QVector<Entry> bitmaps_;
  double dotting_time_us_ = 0;
  float current_pwm_ = 0;
};
