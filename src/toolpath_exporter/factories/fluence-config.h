#pragma once

// Developer-only fluence / CO2-tube compensation configuration, ported from the
// laser-phy-simulator (tools/generate-fcode.ts `Options` and src/core/types.ts
// `DEFAULT_MACHINE`). These are experimental tuning knobs for pre-compensating
// the tube's power-envelope ramp when emitting galvo (Promark) marks; they are
// deliberately kept OUT of the user-facing Config. The exporter owns the values
// (see toolpath-exporter-fcode.cpp) and pushes them to the laser factories.
//
// Units: mm, seconds, mm/s and mm/min as noted (feed rates reach `proc` in
// mm/min; the physics math runs in mm/s).

// Path strategy (generate-fcode.ts --strategy). `Optimized` is the measured-best
// recipe: adaptive + carryover + tail-comp + edge-ext (normalized in the
// emitter).
enum class FluenceStrategy {
  Baseline,   // one mark per run, stock delays
  RampComp,   // average-preserving speed segmentation vs the charge ramp
  Optimized,  // adaptive + carryover + tail-comp + edge-ext
};

// CO2 tube power-envelope ramp (types.ts DEFAULT_MACHINE, "laser config"). Time
// for the output to ramp LINEARLY across full scale (0 -> 100%) on gate open /
// close; the envelope is slope-limited toward its target.
struct LaserPhysicsConfig {
  double laser_ramp_up_s = 0.00068;
  double laser_ramp_down_s = 0.0001;
};

// Baseline / Promark hardware config. Holds the (formerly compile-time)
// PromarkJobConfig values (swiftray src/constants.h), now dynamically adjustable
// and pushed to the execution end as {23} Promark commands. Grouped by the
// Promark API call that consumes them:
//   * motion ctrl        -> lcs_set_jump_speed_ctrl / _mark_speed_ctrl / _delay_mode
//   * laser/scanner delay -> lcs_set_laser_delays / lcs_set_scanner_delays
//   * axis config         -> saved on the execution end for MoveAxis + timing
struct FluenceBaselineConfig {
  // --- motion ctrl (setUpTaskCtrl) ---
  double jump_speed_mm_s = 4000;   // lcs_set_jump_speed_ctrl (also the fluence
                                   // emitter's jump/travel speed)
  double mark_speed_ctrl = 1000;   // lcs_set_mark_speed_ctrl
  int jump_delay_min = 200;        // lcs_set_delay_mode min (us)
  int jump_delay_max = 400;        // lcs_set_delay_mode max (us)
  int jump_delay_limit = 10;       // lcs_set_delay_mode limit
  // --- laser/scanner delays (setUpTaskList) ---
  int laser_on_delay_us = -100;    // lcs_set_laser_delays on (may be negative)
  int laser_off_delay_us = 100;    // lcs_set_laser_delays off
  int scanner_mark_delay_us = 100;    // lcs_set_scanner_delays mark
  int scanner_polygon_delay_us = 50;  // lcs_set_scanner_delays polygon
  // --- axis config (saved for MoveAxis + time estimate) ---
  double z_pulse_per_mm = 1600;
  double z_pulse_per_sec = 4800;
  double a_pulse_per_mm = 63;
  double a_pulse_per_sec = 3200;
};

// Fluence optimization levers (generate-fcode.ts "optimization config").
struct FluenceOptimizationConfig {
  FluenceStrategy strategy = FluenceStrategy::Baseline;
  bool adaptive = false;    // nominal-fluence speed adaptation per run
  bool carryover = false;   // track the tube envelope across runs (implies adaptive)
  bool tail_comp = false;   // discharge tail finishes the run (implies adaptive)
  bool edge_ext = false;    // extend each run by edge_ext_mm on both ends
  int cross_pass = 1;       // N serpentine passes, per-pass speed xN
  int ramp_steps = 3;       // speed steps per ramp zone (Optimized defaults to 5)
  bool comp_down = false;   // ramp-comp only: mirrored slow-down zone at run ends
  // edge-ext length (mm). generate-fcode.ts uses half a pixel pitch; exposed as
  // a free mm value here ("convert to mm with more freedom").
  double edge_ext_mm = 0.0;
};

// Galvo command-list buffer (types.ts DEFAULT_MACHINE, "optional list config").
struct GalvoListConfig {
  int galvo_list_capacity = 8192;  // max commands per galvo list buffer
};

// Bundle passed to the laser factories.
struct FluenceConfig {
  LaserPhysicsConfig physics;
  FluenceBaselineConfig baseline;
  FluenceOptimizationConfig optimization;
  GalvoListConfig list;
};
