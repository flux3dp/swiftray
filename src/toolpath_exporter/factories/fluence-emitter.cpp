#include "fluence-emitter.h"
#include "toolpath_exporter/generators/fcode-generator.h"
#include <QLineF>
#include <algorithm>
#include <cmath>

void FluenceEmitter::set_config(const FluenceConfig& config) {
  config_ = config;
  FluenceOptimizationConfig& o = config_.optimization;
  // Normalize like generate-fcode.ts parseArgs: the Optimized recipe turns on
  // all levers (5 ramp steps unless overridden), and carryover/tail-comp both
  // plan against the adaptive lead-in.
  if (o.strategy == FluenceStrategy::Optimized) {
    o.adaptive = true;
    o.carryover = true;
    o.tail_comp = true;
    o.edge_ext = true;
    if (o.ramp_steps == 3) o.ramp_steps = 5;
  }
  if (o.carryover || o.tail_comp) o.adaptive = true;
}

void FluenceEmitter::set_run_params(double power_pct,
                                    double nominal_speed_mm_min) {
  power_pct_ = power_pct;
  int passes = std::max(1, config_.optimization.cross_pass);
  v_nom_mm_s_ = (nominal_speed_mm_min / 60.0) * passes;
  slope_up_ = config_.physics.laser_ramp_up_s > 0
                  ? 100.0 / config_.physics.laser_ramp_up_s
                  : 0.0;
  slope_dn_ = config_.physics.laser_ramp_down_s > 0
                  ? 100.0 / config_.physics.laser_ramp_down_s
                  : 0.0;
}

bool FluenceEmitter::active() const {
  const FluenceOptimizationConfig& o = config_.optimization;
  return o.strategy != FluenceStrategy::Baseline || o.adaptive || o.carryover ||
         o.tail_comp || o.edge_ext || o.cross_pass > 1;
}

int FluenceEmitter::cross_pass() const {
  return std::max(1, config_.optimization.cross_pass);
}

void FluenceEmitter::begin_block() {
  env_ = 0;
  has_last_ = false;
  current_pwm_ = -1;
}

void FluenceEmitter::set_pwm(float pct) {
  if (current_pwm_ != pct) {
    proc_->set_toolhead_pwm(pct);
    current_pwm_ = pct;
  }
}

void FluenceEmitter::mark(const QPointF& p, double v_mm_s) {
  set_pwm(100);
  proc_->moveto(
      NamedArgs().rx(p.x()).ry(p.y()).rf(std::max(v_mm_s, 1e-6) * 60.0));
}

void FluenceEmitter::travel_to(const QPointF& p) {
  // Jump to the run start at the configured jump speed (matches the simulator's
  // SET_JUMP_SPEED(jumpSpeed) before each JUMP_ABS, and the envelope-decay time
  // computed from the same jump speed in emit_run).
  travel_at(p, config_.baseline.jump_speed_mm_s);
}

void FluenceEmitter::travel_at(const QPointF& p, double v_mm_s) {
  // Gate-off jump at speed v. On the galvo the jump speed comes from
  // lcs_set_jump_speed_ctrl (driven by the {23,8} jump-speed command), NOT from
  // a per-move feed, so set the jump speed and then jump.
  proc_->set_promark_jump_speed_ctrl(std::max(v_mm_s, 1e-6));
  set_pwm(0);
  proc_->moveto(NamedArgs().rx(p.x()).ry(p.y()).set_is_travel());
}

void FluenceEmitter::emit_run(const QPointF& a_in, const QPointF& b_in) {
  const FluenceOptimizationConfig& o = config_.optimization;
  const double P = power_pct_;
  const double vNom = v_nom_mm_s_;
  const int N = std::max(1, o.ramp_steps);
  const double slopeUp = slope_up_;
  const double slopeDn = slope_dn_;
  const double jumpSpeed =
      config_.baseline.jump_speed_mm_s > 0 ? config_.baseline.jump_speed_mm_s : 1;
  const bool segmented =
      (o.strategy == FluenceStrategy::RampComp) || o.adaptive;

  QPointF a = a_in;
  QPointF b = b_in;
  double L = QLineF(a, b).length();

  // edge-ext: extend the run by edge_ext_mm on both ends (along its direction).
  if (o.edge_ext && L > 0 && o.edge_ext_mm > 0) {
    QPointF u = (b - a) / L;
    a = a - u * o.edge_ext_mm;
    b = b + u * o.edge_ext_mm;
    L = QLineF(a, b).length();
  }
  const QPointF u = L > 0 ? (b - a) / L : QPointF(0, 0);
  auto at = [&](double d) { return a + u * std::min(d, L); };

  // Jump to the run start; the envelope decays over the jump time.
  if (has_last_) {
    double jumpT = QLineF(last_, a).length() / jumpSpeed;
    env_ = std::max(0.0, env_ - slopeDn * jumpT);
  } else {
    env_ = 0;  // block start: the gantry leg dwarfs the decay time
  }
  travel_to(a);
  const double env0 = o.carryover ? env_ : 0.0;

  auto finish = [&](const QPointF& p, double envEnd) {
    last_ = p;
    has_last_ = true;
    env_ = std::min(P, envEnd);
    set_pwm(0);
  };

  if (L <= 0) {
    // Degenerate zero-length run: nothing to traverse (no dwell primitive).
    finish(a, env0);
    return;
  }

  if (!segmented) {
    // baseline: one mark at the nominal speed.
    mark(b, vNom);
    finish(b, env0 + slopeUp * L / vNom);
    return;
  }

  if (o.strategy == FluenceStrategy::RampComp) {
    // Wave-3 average-preserving segmentation: total run time stays L/vNom, the
    // plateau speeds up to fit the ramp zones.
    const double tUpS = config_.physics.laser_ramp_up_s * P / 100.0;
    const double tDnS =
        o.comp_down ? config_.physics.laser_ramp_down_s * P / 100.0 : 0.0;
    const double denom = L / vNom - 0.5 * (tUpS + tDnS);
    const double vPl = denom > 0 ? std::min(L / denom, 2 * vNom) : 0.0;
    const double upLen = vPl * tUpS / 2;
    const double dnLen = vPl * tDnS / 2;
    if (vPl <= 0 || L - upLen - dnLen < 0) {
      // Degrade: whole-run stepped speeds at unchanged total time.
      double d = 0;
      for (int i = 1; i <= N; i++) {
        double v = (2 * vNom * (i - 0.5)) / N;
        d += (v * L) / (N * vNom);
        mark(i == N ? b : at(d), v);
      }
      finish(b, (slopeUp * L) / vNom);
      return;
    }
    double d = 0;
    for (int i = 1; i <= N; i++) {
      double v = ((i - 0.5) / N) * vPl;
      d += (v * tUpS) / N;
      mark(at(d), v);
    }
    mark(o.comp_down ? at(L - dnLen) : b, vPl);
    if (o.comp_down) {
      d = L - dnLen;
      for (int i = 1; i <= N; i++) {
        double v = ((N - i + 0.5) / N) * vPl;
        d += (v * tDnS) / N;
        mark(i == N ? b : at(d), v);
      }
    }
    finish(b, P);
    return;
  }

  // adaptive-speed: deposit the NOMINAL fluence (P/vNom per mm) everywhere.
  // Stepped lead-in from env0 to envEnd (equal-time marks); returns its length.
  auto emitLeadIn = [&](double envEnd) -> double {
    double tau = (envEnd - env0) / slopeUp / N;
    double d = 0;
    for (int i = 1; i <= N; i++) {
      double v = (vNom * (env0 + slopeUp * (i - 0.5) * tau)) / P;
      d += v * tau;
      mark(at(d), v);
    }
    return d;
  };
  // Envelope-tracking discharge jumps from envFrom, ending exactly at b. Like
  // the simulator, the gate is closed and the run is finished with jumps at the
  // decaying envelope speeds (the tube's discharge tail deposits the fluence).
  auto emitTail = [&](double fromLen, double envFrom) {
    double tau = envFrom / slopeDn / N;
    double d = fromLen;
    for (int i = 1; i <= N; i++) {
      double v = std::max((vNom * (envFrom - slopeDn * (i - 0.5) * tau)) / P, 1e-6);
      d += v * tau;
      travel_at(i == N ? b : at(d), v);
    }
  };

  const double tRem = std::max(0.0, (P - env0) / slopeUp);
  const double zoneLen = ((vNom * (env0 + P)) / (2 * P)) * tRem;
  const double tailLenFull = (vNom * P) / (2 * slopeDn);

  if (o.tail_comp) {
    if (env0 < P - 1e-9 && zoneLen + tailLenFull <= L) {
      emitLeadIn(P);
      mark(at(L - tailLenFull), vNom);
      emitTail(L - tailLenFull, P);
      finish(b, 0);
      return;
    }
    if (env0 >= P - 1e-9 && tailLenFull <= L) {
      mark(at(L - tailLenFull), vNom);
      emitTail(L - tailLenFull, P);
      finish(b, 0);
      return;
    }
    const double peakSq = ((2 * P * L) / vNom + (env0 * env0) / slopeUp) /
                          (1 / slopeUp + 1 / slopeDn);
    const double envPeak = std::sqrt(std::max(0.0, peakSq));
    if (envPeak > env0 + 1e-9) {
      double dMark = emitLeadIn(std::min(P, envPeak));
      emitTail(dMark, std::min(P, envPeak));
      finish(b, 0);
      return;
    }
    // Degenerate: fall through to the plain adaptive paths below.
  }

  if (env0 >= P - 1e-9) {
    // Tube already at setpoint: plain nominal mark, no lead-in needed.
    mark(b, vNom);
    finish(b, P);
    return;
  }
  if (zoneLen >= L) {
    // Whole run inside the charge zone: N equal-time steps covering L.
    const double A = (vNom * slopeUp * N * N) / (2 * P);
    const double B = (vNom * N * env0) / P;
    const double tau = (-B + std::sqrt(B * B + 4 * A * L)) / (2 * A);
    double d = 0;
    for (int i = 1; i <= N; i++) {
      double v = (vNom * (env0 + slopeUp * (i - 0.5) * tau)) / P;
      d += v * tau;
      mark(i == N ? b : at(d), v);
    }
    finish(b, env0 + slopeUp * N * tau);
    return;
  }
  // Lead-in over tRem, then plateau at vNom.
  emitLeadIn(P);
  mark(b, vNom);
  finish(b, P);
}
