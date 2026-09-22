#pragma once

#include <QPointF>
#include <QRectF>
#include <QtGlobal>
#include <cstdint>
#include <functional>
#include <initializer_list>

class FCodeGenerator;

/**
 * Galvo (BSL) list emission for HEXA II, per HX2_GALVO_PROTOCOL.md §19.
 *
 * Everything handed to the galvo board travels in the FLUX fcode container as a
 * run of command byte 23, each carrying one record:
 *
 *     uint16 LE opcode | uint8 paramCount | paramCount x float64 LE
 *
 * The opcode table is the simulator's (§4.2); FLUX extensions start at 0x20.
 * Opcodes 17/18 are deliberately absent -- the protocol forbids them.
 */
enum class GalvoOp : uint16_t {
  JUMP_ABS = 1,            // x, y
  JUMP_REL = 2,            // dx, dy
  MARK_ABS = 3,            // x, y
  MARK_REL = 4,            // dx, dy
  LASER_ON = 5,            // period(us) -- this is a dot
  LONG_DELAY = 6,          // delay(us)
  SET_JUMP_SPEED = 7,      // mm/s
  SET_MARK_SPEED = 8,      // mm/s
  SET_LASER_DELAYS = 9,    // on(us), off(us)  -- may be negative
  SET_SCANNER_DELAYS = 10, // mark(us), polygon(us)
  SET_LASER_POWER = 11,    // 0-100
  SET_LASER_PULSES = 12,   // period(us), pulseLength(us), mopaPulse(ns)
  SET_STANDBY = 13,        // period(us), width(us)
  ENABLE_LASER = 14,       // moDelay(us)
  DISABLE_LASER = 15,      // moDelay(us)
  SET_WOBBLE = 16,         // transversal, longitudinal, space, mode
  SET_END_OF_LIST = 19,    // estimated list run time (ms) -- see §20.1
  // FLUX extensions (0x20 and up; 0x14-0x1F stay reserved for the simulator)
  AXIS_MOVE = 0x21,        // A axis only; Z never reaches the galvo board
  SET_IO = 0x22,           // mask, value
};

/** lcs_set_wobble_mode()'s mode argument. */
enum class GalvoWobbleMode : int {
  DISABLE = -1,
  WHEEL = 0,
};

/**
 * The parameter set a list's prologue carries. Everything here is re-sent at the
 * head of every list: §19.3 requires each list to stand on its own, because the
 * board frames every list with disable_laser and there is no state replay on the
 * bsl side to lean on.
 */
struct GalvoParams {
  // --- written into the prologue -------------------------------------------
  double jump_speed_mm_s = 4000;
  double mark_speed_mm_s = 1000;
  double laser_on_delay_us = -100;
  double laser_off_delay_us = 100;
  double scanner_mark_delay_us = 100;
  double scanner_polygon_delay_us = 50;
  double power_pct = 0;
  // opcode 12, all three of them. The third is a real Mopa pulse width in ns
  // (lcsApi.h:1133, range 1..65535), not the flag the simulator's 2-parameter
  // definition made it look like -- see the change notice at the head of §19.
  double pulse_period_us = 31.25;  // 32 kHz
  double pulse_length_us = 0;
  double mopa_pulse_ns = 1;
  // CO2 heads have no separate power input: the duty cycle is the power, so
  // pulse_length_us is derived from power_pct and re-sent whenever power moves.
  // Mopa heads carry their own width and are set once per layer.
  bool derive_pulse_from_power = true;
  // Standby ("pre ionization") train. bsl only calls set_standby_list when a
  // file asks for it, so leaving this out means the board is never told at all.
  // CO2 only: pre-ionization is a CO2 notion, and the code this replaces was
  // guarded by if (!isMopaHead()).
  bool emit_standby = true;
  double standby_period_us = 100;
  // set_standby_list takes this as a uint32_t (lcsApi.h:1116), so it is rounded
  // to a whole microsecond and floored at 1 -- anything under 0.5 would reach
  // the SDK as 0, outside its documented 0.021~1365 range.
  double standby_width_us = 1;
  // Wobble is only emitted when the mode is not DISABLE.
  GalvoWobbleMode wobble_mode = GalvoWobbleMode::DISABLE;
  double wobble_transversal_mm = 0;
  double wobble_longitudinal_mm = 0;
  double wobble_space_mm = 0;

  // --- time estimation only, never emitted ---------------------------------
  // Multiplier for the extra path length wobble adds. 1 when wobble is off.
  double wobble_k = 1;
  // Non-zero switches marking to dotting: jump to the point, then LASER_ON.
  double dotting_time_us = 0;

  // The delay-mode window, as bsl hands it to the card. These are not settings:
  // set_delay_mode is a CONTROL instruction with no LIST form (§16-C), so it is
  // called once per job with compile-time constants (bsl_controller.cpp:440),
  // and a file cannot ask for anything else. They live here only because §14
  // requires this estimate to use the numbers the card is actually running.
  static constexpr double kJumpDelayMinUs = 200;
  static constexpr double kJumpDelayMaxUs = 400;
  // The jump length at which the delay reaches its maximum, in mm.
  static constexpr double kJumpDelayLimitMm = 10;

  /**
   * §14's jump delay, for a jump of `distance_mm`.
   *
   * bsl enables the variable window (set_delay_mode's VarPoly is true), so the
   * delay is not the flat mean it was taken for: it rises with the jump from
   * the minimum to the maximum and saturates at the length limit. That matters
   * most where the jumps are short and many -- a stitched raster hops a tooth's
   * width at a time, a few millimetres, which the window charges near its floor
   * rather than its middle.
   */
  double jump_delay_ms(double distance_mm) const {
    const double reach =
        qBound(0.0, distance_mm / kJumpDelayLimitMm, 1.0);
    return (kJumpDelayMinUs +
            (kJumpDelayMaxUs - kJumpDelayMinUs) * reach) /
           1000;
  }
  double laser_delay_ms() const {
    return (laser_off_delay_us - laser_on_delay_us) / 1000;
  }

  /** opcode 12's param[1] for the current power, on a head that derives it. */
  double effective_pulse_length_us() const {
    return derive_pulse_from_power
               ? qMax(0.021, pulse_period_us * power_pct / 100)
               : pulse_length_us;
  }
};

/**
 * Builds the galvo command stream for one job.
 *
 * Coordinates arrive in machine mm (bed-aligned, the same frame the gantry
 * commands use) and leave as field-local mm relative to the block's lens centre
 * -- §3 moves that conversion to swiftray. Mirroring is not applied here; it
 * belongs at the SDK boundary (§18.6), so what lands in the file stays
 * bed-aligned.
 *
 * Lifecycle:
 *
 *     beginBlock(centre)      // gantry is already parked there
 *       beginList()           // prologue
 *       ... moveTo / setLaserOn ...
 *       endList()             // DISABLE_LASER + SET_END_OF_LIST(ms)
 *     endBlock()
 */
class GalvoListWriter {
 public:
  explicit GalvoListWriter(FCodeGenerator* gen) : gen_(gen) {}

  GalvoParams& params() { return params_; }
  const GalvoParams& params() const { return params_; }

  /**
   * Open a galvo block centred on `field_centre_mm` (machine mm). Everything
   * emitted until endBlock() is expressed relative to that point.
   *
   * `on_first_record` runs immediately before the block's first record, so the
   * caller can park the gantry there. Deferring it means a block that turns out
   * to hold nothing costs no head travel at all -- which is most of them once a
   * sparse drawing is tiled across a large bed.
   */
  void beginBlock(const QPointF& field_centre_mm,
                  double half_field_mm,
                  std::function<void()> on_first_record = {});
  void endBlock();
  /**
   * False while `on_first_record` runs: the gantry commands it writes must reach
   * the generator instead of being folded back into the galvo list.
   */
  bool in_block() const { return in_block_ && !running_block_start_; }
  /** True once the block has actually written something. */
  bool block_started() const { return block_started_; }
  bool list_open() const { return list_open_; }

  /**
   * Arm a list. The §19.3 prologue is written when the first geometry command
   * arrives, so a block that turns out to be empty costs nothing; either way the
   * prologue is the first thing in the list.
   */
  void beginList();
  /**
   * Close a list: DISABLE_LASER(0) then SET_END_OF_LIST(estimated ms). Does
   * nothing when the armed list never received any geometry.
   */
  void endList();

  /**
   * Restrict emission to `rect` (machine mm). Segments are clipped against it,
   * so a path crossing the edge is cut rather than dropped. An empty rect
   * clears the restriction.
   */
  void set_clip_rect(const QRectF& rect_mm) { clip_rect_ = rect_mm; }
  void clear_clip_rect() { clip_rect_ = QRectF(); }

  /** Laser on -> subsequent moves mark (or dot); off -> they are travels. */
  void set_laser_on(bool on) { laser_on_ = on; }
  bool laser_on() const { return laser_on_; }

  /** Both take effect immediately, mid-list if need be (§5). */
  void set_mark_speed(double mm_s);
  /** On a CO2 head this also re-sends SET_LASER_PULSES, which carries the duty. */
  void set_power(double pct);

  /** Move to `x_mm`, `y_mm` in machine mm. NaN keeps that axis. */
  void moveTo(double x_mm, double y_mm);
  /**
   * Bring the mirrors back to the middle of the field before the list closes,
   * so the galvo is left centred rather than wherever the last mark happened to
   * end. Not clipped -- the block owns its own centre by construction -- and
   * does nothing if the block never wrote anything.
   */
  void returnToCentre();

  /** Estimated run time of the list being built, in ms (§14). */
  double list_time_ms() const { return list_time_ms_; }
  /** Galvo distance covered since beginBlock(), for the travel_dist metadata. */
  double block_distance_mm() const { return block_distance_mm_; }

  /** Number of records written into the list being built. */
  int list_command_count() const { return list_command_count_; }
  /**
   * Split the current list after this many records (0 = never). The protocol
   * sets no upper bound (§19.7) and bsl no longer splits on its own (§20.2), so
   * this is off unless a machine profile asks for it.
   */
  void set_max_commands_per_list(int n) { max_commands_per_list_ = n; }

  /** Field-local points that fell outside +/- half_field, counted for the
   *  whole job (never reset; take differences to scope it). */
  int out_of_field_count() const { return out_of_field_count_; }

 private:
  void emit(GalvoOp op, std::initializer_list<double> params);
  /** Write the prologue if the armed list has not started yet. */
  void ensureListOpen();
  /** Run the block-start callback, once per block. */
  void ensureBlockStarted();
  /** Move the board to `p` (machine mm) with a jump, if it is not there. */
  void jumpTo(const QPointF& p);
  void markTo(const QPointF& p);
  void emitPulses();
  QPointF toField(const QPointF& p);
  /**
   * Liang-Barsky clip of p0->p1 against clip_rect_. Returns false when the
   * segment lies entirely outside; otherwise p0/p1 are moved to the crossings.
   */
  bool clipSegment(QPointF& p0, QPointF& p1) const;

  FCodeGenerator* gen_ = nullptr;
  GalvoParams params_;

  bool in_block_ = false;
  bool block_started_ = false;
  bool running_block_start_ = false;
  std::function<void()> on_block_start_;
  bool list_armed_ = false;
  bool list_open_ = false;
  QPointF field_centre_;
  double half_field_mm_ = 55;
  QRectF clip_rect_;

  // Logical head position in machine mm: where the toolpath thinks it is,
  // whether or not anything was emitted for the last move.
  QPointF cur_;
  bool cur_valid_ = false;
  // Board position in machine mm: where the last emitted command left it.
  QPointF emitted_;
  bool emitted_valid_ = false;
  bool laser_on_ = false;

  double list_time_ms_ = 0;
  double block_distance_mm_ = 0;
  int list_command_count_ = 0;
  int max_commands_per_list_ = 0;
  int out_of_field_count_ = 0;
};
