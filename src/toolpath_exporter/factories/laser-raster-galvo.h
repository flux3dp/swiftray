#pragma once

#include <QImage>
#include <QtGlobal>
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
   * How a run crosses a seam, mirroring the four overlap line strategies in
   * laser-phy-simulator (lineRasteringTiledGenerator.ts). The first three hand
   * each stretch to a single block and differ only in what the ownership phase
   * is made of:
   *
   *  - Scanlines: the scan line alone, so a whole line belongs to one block and
   *    the seam alternates row by row;
   *  - Segments: the position along the line alone, with the candidate order
   *    reversed on the lines that are scanned backwards, so the teeth stand at
   *    the same places every line but change hands alternately;
   *  - Checkerboard: both, so the teeth themselves shift along row by row.
   *
   * Pwm is the fourth and not an ownership rule at all: every block marks the
   * whole band, each at its own share of the power. On a CO2 head that share is
   * the duty cycle, which is what the simulator modulates; on a Mopa head it is
   * the power input. The shares are normalised, so a site two blocks cross
   * still receives one full exposure -- a smooth fade rather than teeth.
   */
  enum class RunEmission { Segments, Scanlines, Checkerboard, Pwm };

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
    /**
     * The band and the falloff are settled per path, not once: the simulator
     * keeps dotBlendOverlapMm/dotBlendProfile apart from lineBlendOverlapMm/
     * lineBlendProfile, and defaults them differently. Dots can afford a finer
     * staircase than runs, which pay a jump at every step of it.
     *
     * Total band width, mm; half of it lies each side of a shared edge.
     */
    double dot_overlap = 10;
    BlendProfile dot_profile = BlendProfile::Granular;
    double line_overlap = 10;
    BlendProfile line_profile = BlendProfile::SuperGranular;
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
     * jump. On a solid fill crossing every seam, 2 mm teeth cost 39% of the run
     * time and 10 mm ones 17%. Nothing is lost either way -- the marked length
     * is identical to a hard seam -- so this trades time for how visible a
     * misplaced seam is. 2 mm is the simulator's own default.
     */
    double segment_length = 2;
    RunEmission line_emission = RunEmission::Checkerboard;
    /**
     * Two-pass seam, the simulator's 'dot-blend-line-core'. The core -- the
     * cell drawn back by half the dot overlap on every shared side -- is
     * engraved as runs at full exposure, and only the band around it is laid as
     * dots, blended by the same density rule a dithered image uses. It is the
     * dot band throughout: the line knobs take no part in it, which is why the
     * core is measured off the dot overlap.
     *
     * This is the one answer to a run having no tone to fade: give the band
     * tone by turning it into dots. It costs a dot per dark site across the
     * band rather than a mark per stretch.
     */
    bool band_dots = false;
    /**
     * What the band's dots are laid with, the simulator's processDot block.
     * The hybrid is the only thing that reads it: its dots stitch a seam
     * between two line-rastered cores, so they are not bound to the settings
     * that engraved those cores -- a different power, a coarser grid and a
     * longer pulse are exactly how the band is made to read at the same depth.
     *
     * Every member falls back to what the layer already carries, so a caller
     * that names none of them gets the behaviour this had before the block
     * existed. They travel in a list of their own: the prologue is where a
     * galvo list states its speeds, powers and delays, so the way to run the
     * band differently is to give it its own list.
     */
    struct DotProcess {
      /** Percent. Zero keeps the layer's power. */
      double power_pct = 0;
      /** mm between dots. Zero walks the image's own pixel grid instead. */
      double pitch_mm = 0;
      /** Microseconds. Zero keeps the layer's pulse period. */
      double pulse_period_us = 0;
      /**
       * Dwell for one dot, microseconds. Zero derives it from the pitch and
       * the mark speed -- how long a line pass would have spent over the same
       * ground.
       */
      double pulse_on_time_us = 0;
      /** mm/s. Zero keeps the prologue's jump speed. */
      double jump_speed_mm_s = 0;
      /** Microseconds. NaN keeps the prologue's delay. */
      double laser_on_delay_us = qQNaN();
      double laser_off_delay_us = qQNaN();
    };
    DotProcess dot_process;
  };

  explicit LaserRasterGalvoFactory(const FactoryKwargs& kwargs) noexcept;
  void set_blend(const Blend& blend) { blend_ = blend; }
  /**
   * The region a block samples: its cell, grown into every shared seam. Which
   * band depends on what is being emitted, so this follows the pass that is
   * running -- dots reach further or less far than runs whenever the two
   * overlaps differ.
   */
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
  /** One cell that samples a site, and how much of it it claims. */
  struct Candidate {
    int col;
    int row;
    double weight;
  };

  /**
   * Every cell whose band covers this site, in a fixed order, with the sum of
   * their raw weights. Whoever asks gets the same list, which is what lets the
   * blocks agree without talking to each other.
   */
  double collectCandidates(double x, double y, QVector<Candidate>* out) const;

  /** One scan line: a row of the image, or a column when transposed. */
  void emitLine(const Entry& entry, int index, int threshold, bool reversed,
                bool transposed, float speed);
  /**
   * The hybrid's second pass: dots across the band the runs stopped short of.
   *
   * Walks a grid of its own rather than the image's pixels, because the band's
   * dots answer to the seam and not to the picture -- the simulator gives them
   * their own pitch for the same reason. The grid is anchored at the machine
   * origin so that neighbouring blocks land on the same sites and can agree on
   * who owns each one.
   */
  void emitBandDots(const Entry& entry, int threshold, float speed);
  /** Whether `mm` falls on a dark pixel of `entry`. */
  bool sampleAt(const Entry& entry, const QPointF& mm, int threshold) const;
  void setPwm(float pwm);
  /** Power for the marks that follow, as a percentage of full. */
  void setPower(double pct);
  double profileDensity(double progress) const;
  /** The nominal cell of a lattice position. */
  QRectF cellAt(int col, int row) const;
  /** A cell grown into every seam it shares -- what that block samples. */
  QRectF bandAt(int col, int row) const;
  /** The band width and falloff of whichever pass is running. */
  double activeOverlap() const;
  BlendProfile activeProfile() const;
  /**
   * Whether the pass under way blends at all: the lattice has to be split, and
   * the band that pass works to has to have a width. The two overlaps are
   * independent, so a layer can fade its dots across the seam while its runs
   * butt up hard against it, or the other way round.
   */
  bool blendActive() const;
  /**
   * The full-exposure core: this block's cell drawn back by half the overlap on
   * every side it shares. Outside edges keep the cell.
   */
  QRectF coreRect() const;
  /** This block's share of a site, normalised against everyone who samples it. */
  double blockWeight(double x, double y) const;
  /** How much of this site belongs to that cell, before normalising. */
  double rawWeight(int col, int row, double x, double y) const;
  /**
   * Whether this block, of all that sample it, is the one that lays this site.
   * `reversed` walks the candidates the other way round, which hands the site
   * to the block at the other end of the distribution.
   */
  bool ownsSite(double x, double y, double target, bool reversed = false) const;
  /** Ordered-screen target: a dithered image changes owner site by site. */
  double siteTarget(int column, int row) const;
  /**
   * Coarse target: a run holds its owner for a stretch of the scan line.
   * `along` is the coordinate down the scan, `line` the global index of the
   * scan line itself -- global because neighbouring blocks have to draw the
   * same target for the same place, and they only walk the same grid.
   */
  double runTarget(double along, int line) const;

  Blend blend_;
  /**
   * Whether the pass under way is laying dots. The two paths blend against
   * different bands, so every weight, band and progress has to know which one
   * is asking.
   */
  bool dot_pass_ = false;
  QVector<Entry> bitmaps_;
  /** The layer's own power, which the pwm fade takes its shares of. */
  double base_power_pct_ = 0;
  double current_power_pct_ = 0;
  double dotting_time_us_ = 0;
  float current_pwm_ = 0;
};
