#ifndef PROMARK_TIMING_H
#define PROMARK_TIMING_H

#include <QString>
#include <QStringList>

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "constants.h"

/**
 * Timing model of the Promark (BSL/LCS) controller.
 *
 * Two places predict how long a job takes and they must agree, because one is the progress bar's
 * denominator and the other its numerator:
 *   - MachineJob::calcTotalTime()          -- whole job, from the GCode, before sending anything
 *   - BSLMotionController::estimated_time_ -- per list, while streaming (also decides when to swap
 *                                             the double-buffered lists)
 * Everything they share lives here so they cannot drift apart.
 */
namespace PromarkTiming {

/**
 * Time an axis spends actually travelling `pulses`, in ms -- the ramp only, no command overhead.
 *
 * The controller ramps from start_speed up to run_speed over acc_time and symmetrically back down.
 * A move too short to reach run_speed is therefore a pure triangle whose duration grows with
 * sqrt(pulses), not linearly -- with the Z ramp (Z_START_PULSE_PER_SEC 10 -> 4800 pulse/s over
 * 255ms) a full ramp needs ~1227 pulses, i.e. 0.77mm of Z. Every layer step of a 3D carving job is
 * inside that regime (0.1mm = 160 pulses): 333 steps of 0.1mm travel for ~61s, ~6x the ~11s a
 * constant-speed model predicts.
 */
inline double axisTravelTimeMs(double pulses, double run_speed, double start_speed,
                               double acc_time_ms) {
  pulses = std::fabs(pulses);
  if (pulses <= 0 || run_speed <= 0) return 0;
  if (acc_time_ms <= 0 || run_speed <= start_speed) {
    return 1000.0 * pulses / run_speed;
  }
  const double acc = (run_speed - start_speed) / (acc_time_ms / 1000.0); // pulse/s^2
  const double ramp_pulses =
      (run_speed * run_speed - start_speed * start_speed) / (2 * acc);
  if (pulses >= 2 * ramp_pulses) {
    // Trapezoid: accelerate, cruise at run_speed, decelerate.
    return 2 * acc_time_ms + 1000.0 * (pulses - 2 * ramp_pulses) / run_speed;
  }
  // Triangle: run_speed is never reached, the move peaks at sqrt(start^2 + acc * pulses).
  const double peak_speed = std::sqrt(start_speed * start_speed + acc * pulses);
  return 1000.0 * 2 * (peak_speed - start_speed) / acc;
}

/**
 * Duration of one lcs_set_axis_move(), in ms: the travel plus AXIS_MOVE_OVERHEAD_MS.
 *
 * The overhead dominates the short moves a 3D carving job is made of -- a 0.001mm Z step travels
 * 1.6 pulses in ~18ms but costs ~70ms on the machine -- so it is charged per command, including a
 * move the pulse quantisation rounds down to nothing: the list instruction is issued either way.
 * Both callers only reach this for a G-code that really does emit one.
 */
inline double axisMoveTimeMs(double pulses, double run_speed, double start_speed,
                             double acc_time_ms) {
  return PromarkJobConfig::AXIS_MOVE_OVERHEAD_MS +
         axisTravelTimeMs(pulses, run_speed, start_speed, acc_time_ms);
}

/** Duration of a Z (depth) move of the given length in mm. */
inline double zMoveTimeMs(double mm) {
  return axisMoveTimeMs(mm * PromarkJobConfig::Z_PULSE_PER_MM,
                        PromarkJobConfig::Z_PULSE_PER_SEC,
                        PromarkJobConfig::Z_START_PULSE_PER_SEC,
                        PromarkJobConfig::AXIS_ACC_TIME_MS);
}

/** Duration of an A (rotary) move of the given length. */
inline double aMoveTimeMs(double mm) {
  return axisMoveTimeMs(mm * PromarkJobConfig::A_PULSE_PER_MM,
                        PromarkJobConfig::A_PULSE_PER_SEC,
                        PromarkJobConfig::A_START_PULSE_PER_SEC,
                        PromarkJobConfig::AXIS_ACC_TIME_MS);
}

/**
 * The machine parameters a job may override through ";CONFIG KEY=VALUE" comments (emitted by
 * ToolpathExporter::parseParam). They change how long the job actually takes, so the estimate has
 * to follow them instead of using the compiled-in defaults.
 */
struct Config {
  double jump_speed = PromarkJobConfig::JUMP_SPEED;             // mm/s
  int32_t laser_on_delay = PromarkJobConfig::LASER_ON_DELAY;    // us, negative = pre-fire
  int32_t laser_off_delay = PromarkJobConfig::LASER_OFF_DELAY;  // us
  int32_t marking_delay = PromarkJobConfig::MARKING_DELAY;      // us, after the last mark vector
  int32_t corner_delay = PromarkJobConfig::CORNER_DELAY;        // us, between two mark vectors
  int32_t jump_delay_min = PromarkJobConfig::JUMP_DELAY_MIN;    // us
  int32_t jump_delay_max = PromarkJobConfig::JUMP_DELAY_MAX;    // us
  bool first_pulse_killer_enabled = false;
  /**
   * Which time model the job uses, see `Legacy` below. Set by ";CONFIG INNER_ENGRAVING=1", which
   * Beam Studio sends only for a 3D (inner engraving) document; every other job keeps the model
   * that shipped before inner engraving existed.
   */
  bool inner_engraving = false;

  void reset() { *this = Config(); }

  /** Apply one ";CONFIG KEY=VALUE" pair. Returns false for keys that are not timing parameters. */
  bool applyKeyValue(const QString &key, const QString &value) {
    if (key == "JUMP_SPEED") {
      jump_speed = value.toDouble();
    } else if (key == "LASER_ON_DELAY") {
      laser_on_delay = value.toInt();
    } else if (key == "LASER_OFF_DELAY") {
      laser_off_delay = value.toInt();
    } else if (key == "MARKING_DELAY") {
      marking_delay = value.toInt();
    } else if (key == "CORNER_DELAY") {
      corner_delay = value.toInt();
    } else if (key == "JUMP_DELAY_MIN") {
      jump_delay_min = value.toInt();
    } else if (key == "JUMP_DELAY_MAX") {
      jump_delay_max = value.toInt();
    } else if (key == "FIRST_PULSE_KILLER_ENABLED") {
      first_pulse_killer_enabled = value.toInt() != 0 ||
                                   value.compare(QStringLiteral("true"),
                                                 Qt::CaseInsensitive) == 0;
    } else if (key == "INNER_ENGRAVING") {
      inner_engraving = value.toInt() != 0;
    } else {
      return false;
    }
    return true;
  }

  /**
   * Settling time after a jump, in ms. lcs_set_delay_mode() runs in variable mode, so the delay
   * scales with the jump length from jump_delay_min up to jump_delay_max at JUMP_LENGTH_LIMIT_MM.
   */
  double jumpDelayMs(double distance_mm) const {
    double ratio = 1.0;
    if (PromarkJobConfig::JUMP_LENGTH_LIMIT_MM > 0) {
      ratio = std::min(1.0, std::max(0.0, distance_mm / PromarkJobConfig::JUMP_LENGTH_LIMIT_MM));
    }
    return (jump_delay_min + (jump_delay_max - jump_delay_min) * ratio) / 1000.0;
  }

  /**
   * Settling time for a jump whose length the GCode does not carry -- the ";DOT n" / ";JUMP n"
   * summaries the exporter emits for a raster only give a count. Uses the mid-point of the range.
   */
  double averageJumpDelayMs() const { return (jump_delay_min + jump_delay_max) / 2000.0; }

  /** Travel time of a jump, settling excluded. */
  double jumpTravelMs(double distance_mm) const {
    if (jump_speed <= 0) return 0;
    return 1000.0 * distance_mm / jump_speed;
  }

  /** Full cost of a jump of the given length: travel plus settling. */
  double jumpTimeMs(double distance_mm) const {
    return jumpTravelMs(distance_mm) + jumpDelayMs(distance_mm);
  }

  /** Cost of marking one vector at the given speed (mm/s), delays excluded. */
  double markTimeMs(double distance_mm, double speed_mm_s) const {
    if (speed_mm_s <= 0) return 0;
    return 1000.0 * distance_mm / speed_mm_s;
  }

  /** Wait before the first vector of a marking run (laser pre-fire / turn-on). */
  double laserOnDelayMs() const { return std::fabs((double)laser_on_delay) / 1000.0; }

  /** Wait between two consecutive vectors of the same marking run. */
  double cornerDelayMs() const { return corner_delay / 1000.0; }

  /** Wait after the last vector of a marking run, before the galvo may jump away. */
  double markEndDelayMs() const { return (laser_off_delay + marking_delay) / 1000.0; }

  /**
   * Cost of one point emitted by the dotting path, excluding the jump to the point.
   *
   * When enabled, BSLMotionController programs first-pulse suppression to dotting_time + 5us and
   * then emits a laser_on_list(dotting_time). A point is an isolated laser operation, so the
   * configured laser-on and laser-off/marking delays apply to every point rather than to a run of
   * mark vectors. Hardware measurements of STL dot jobs consistently take twice this nominal
   * duration, hence the empirical factor below.
   */
  double dotTimeMs(uint32_t dotting_time_us) const {
    constexpr uint32_t kFirstPulseKillerExtraUs = 5;
    const uint32_t first_pulse_us =
        first_pulse_killer_enabled ? dotting_time_us + kFirstPulseKillerExtraUs : 0;
    constexpr double kMeasuredStlDotTimeFactor = 2.0;
    return kMeasuredStlDotTimeFactor *
           (laserOnDelayMs() + markEndDelayMs() +
            (dotting_time_us + first_pulse_us) / 1000.0);
  }
};

/**
 * Tracks whether we are inside a run of consecutive mark vectors, so the galvo delays land where
 * the controller actually applies them: the laser on delay once at the start of the run, the
 * corner delay at every joint inside it, the laser off + marking delay once at the end.
 */
class MarkSequence {
 public:
  /** Overhead to add before a mark vector, in ms. */
  double beginSegmentMs(const Config &config) {
    if (in_sequence_) return config.cornerDelayMs();
    in_sequence_ = true;
    return config.laserOnDelayMs();
  }

  /** Overhead to add once the run ends -- a jump, an axis move or the end of the job. 0 if idle. */
  double endMs(const Config &config) {
    if (!in_sequence_) return 0;
    in_sequence_ = false;
    return config.markEndDelayMs();
  }

  void reset() { in_sequence_ = false; }

 private:
  bool in_sequence_ = false;
};

/**
 * Split a ";CONFIG ..." line. Returns false if the line is not one. For ";CONFIG RESET" the key is
 * "RESET" and the value empty. The key is upper-cased; the line may be in either case.
 */
inline bool parseConfigLine(const QString &line, QString *key, QString *value) {
  if (!line.startsWith(";CONFIG ", Qt::CaseInsensitive)) return false;
  const QString body = line.mid(8).trimmed();
  const int separator = body.indexOf('=');
  if (separator < 0) {
    *key = body.toUpper();
    value->clear();
    return *key == "RESET";
  }
  *key = body.left(separator).trimmed().toUpper();
  *value = body.mid(separator + 1).trimmed();
  return true;
}


/**
 * The time model Promark jobs used before inner engraving (origin/develop), kept verbatim for every
 * job that is not a 3D document.
 *
 * The model above was built for inner engraving and its dot cost was calibrated on a UV STL job;
 * it also changes what a 2D job is estimated at (per-vector laser delay becomes on / corner /
 * off+marking delays, the jump delay scales with length, a dot carries the laser delays). None of
 * that has been checked against a 2D job on hardware, and neither has this legacy model: it is
 * simply what the shipped estimates were. So a 2D job keeps it unchanged, and `Config::inner_engraving`
 * picks the model, in MachineJob::calcTotalTime and BSLMotionController alike.
 *
 * ⚠️ Known and deliberately preserved: for a dot that is its own G-code line (T > 0, S > 0),
 * develop's MachineJob only counted the jump travel while the controller counted travel + jump
 * delay + dotting time. Both halves are kept as they were.
 */
namespace Legacy {
/** Jump delay, the average of the min / max the card is configured with. */
inline double jumpDelayMs() {
  return (PromarkJobConfig::JUMP_DELAY_MIN + PromarkJobConfig::JUMP_DELAY_MAX) / 2000.0;
}
/** Laser delay charged once per mark vector. */
inline double laserDelayMs() {
  return (PromarkJobConfig::LASER_OFF_DELAY - PromarkJobConfig::LASER_ON_DELAY) / 1000.0;
}
inline double jumpTravelMs(double distance_mm) { return 1000.0 * distance_mm / PromarkJobConfig::JUMP_SPEED; }
inline double jumpTimeMs(double distance_mm) { return jumpTravelMs(distance_mm) + jumpDelayMs(); }
inline double markTimeMs(double distance_mm, double speed_mm_s) {
  return 1000.0 * distance_mm / speed_mm_s + laserDelayMs();
}
/** Axis time by distance only: no ramp, no per-command overhead. */
inline double zMoveTimeMs(double z_mm) {
  return std::fabs(z_mm) * (PromarkJobConfig::Z_PULSE_PER_MM / PromarkJobConfig::Z_PULSE_PER_SEC * 1000.0);
}
inline double aMoveTimeMs(double a_mm) {
  return std::fabs(a_mm) * (PromarkJobConfig::A_PULSE_PER_MM / PromarkJobConfig::A_PULSE_PER_SEC * 1000.0);
}
/** One high-speed raster dot (from the ;DOT summary): a jump delay plus the dotting time. */
inline double rasterDotTimeMs(uint32_t dotting_time_us) { return jumpDelayMs() + dotting_time_us / 1000.0; }
}  // namespace Legacy

}  // namespace PromarkTiming

#endif // PROMARK_TIMING_H
