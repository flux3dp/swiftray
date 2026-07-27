#pragma once

#include "fluence-config.h"
#include <QPointF>

class ToolpathProcessor;

// Ports tools/generate-fcode.ts `emitRun` to swiftray fcode. Given a marked run
// (a straight laser-on segment), it emits the jump-to-start plus a speed-
// segmented mark chain that pre-compensates the CO2 tube's charge/discharge
// ramp so the deposited fluence per mm stays at the nominal power/speed.
//
// Coordinates are machine mm (the caller applies any layer offset). Feed rates
// reach `proc` in mm/min; the physics runs in mm/s. The tube envelope and last
// position are tracked across runs (carryover) and reset per block/tile via
// begin_block().
//
// The tail-comp phase matches the simulator exactly: the marked portion ends
// one discharge-length early and the run is finished with gate-off jumps
// (travel moves at the envelope-tracking speeds), so the tube's discharge tail
// deposits the nominal fluence inside the run.
class FluenceEmitter {
 public:
  void set_processor(ToolpathProcessor* proc) { proc_ = proc; }
  // Store options; normalizes the Optimized recipe and lever implications.
  void set_config(const FluenceConfig& config);
  // power_pct: tube setpoint 0..100. nominal_speed_mm_min: layer nominal feed.
  void set_run_params(double power_pct, double nominal_speed_mm_min);

  // True when the options change emission from a plain single-speed mark, so
  // callers know to route their run through emit_run().
  bool active() const;

  // Number of serpentine passes per block (cross-pass). Per-pass speed is xN so
  // the passes sum to the nominal fluence; callers repeat each block this many
  // times with alternating run direction.
  int cross_pass() const;

  // Reset the tracked envelope + last position (call per block / tile / region).
  void begin_block();

  // Emit a marked run from a to b (machine mm). Leaves the laser off afterwards.
  void emit_run(const QPointF& a, const QPointF& b);

 private:
  void set_pwm(float pct);
  void mark(const QPointF& p, double v_mm_s);        // laser on, feed v (mm/s)
  void travel_to(const QPointF& p);                  // laser off jump (travel speed)
  void travel_at(const QPointF& p, double v_mm_s);   // laser off jump at feed v (mm/s)

  ToolpathProcessor* proc_ = nullptr;
  FluenceConfig config_;
  double power_pct_ = 100;
  double v_nom_mm_s_ = 0;  // nominal mark speed (mm/s), incl. cross-pass factor
  double slope_up_ = 0;    // pct/s toward setpoint while gated
  double slope_dn_ = 0;    // pct/s toward 0 while off

  // Tracked across runs (carryover); reset by begin_block().
  double env_ = 0;         // tube envelope, pct
  bool has_last_ = false;
  QPointF last_;
  float current_pwm_ = -1;
};
