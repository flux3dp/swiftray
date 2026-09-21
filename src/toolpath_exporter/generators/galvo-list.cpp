#include "galvo-list.h"

#include <QDebug>
#include <QLineF>
#include <cmath>

#include "fcode-generator.h"

void GalvoListWriter::emit(GalvoOp op, std::initializer_list<double> params) {
  gen_->write_galvo_command(uint16_t(op), params);
  list_command_count_++;
}

void GalvoListWriter::beginBlock(const QPointF& field_centre_mm,
                                 double half_field_mm,
                                 std::function<void()> on_first_record) {
  if (in_block_) {
    qWarning() << "GalvoListWriter::beginBlock() while a block is open";
    endBlock();
  }
  in_block_ = true;
  block_started_ = false;
  on_block_start_ = std::move(on_first_record);
  field_centre_ = field_centre_mm;
  half_field_mm_ = half_field_mm;
  block_distance_mm_ = 0;
  // The gantry move that parked the head is what put us here; the board itself
  // has no position yet, so the first emitted move has to be absolute (§19.4).
  emitted_valid_ = false;
  cur_ = field_centre_mm;
  cur_valid_ = true;
  laser_on_ = false;
}

void GalvoListWriter::endBlock() {
  if (!in_block_) {
    return;
  }
  endList();
  in_block_ = false;
  on_block_start_ = nullptr;
}

void GalvoListWriter::ensureBlockStarted() {
  if (block_started_) {
    return;
  }
  block_started_ = true;
  if (on_block_start_) {
    running_block_start_ = true;
    on_block_start_();
    running_block_start_ = false;
  }
}

void GalvoListWriter::beginList() {
  if (list_armed_ || list_open_) {
    qWarning() << "GalvoListWriter::beginList() while a list is open";
    endList();
  }
  list_armed_ = true;
  list_open_ = false;
  list_time_ms_ = 0;
  list_command_count_ = 0;
}

void GalvoListWriter::ensureListOpen() {
  if (list_open_ || !list_armed_) {
    return;
  }
  ensureBlockStarted();
  list_open_ = true;
  if (params_.emit_standby) {
    // First in the §19.3 order. Our bsl emits no set_standby_list of its own,
    // so if this is not here the board never gets a standby train.
    emit(GalvoOp::SET_STANDBY,
         {params_.standby_period_us,
          qMax(1.0, std::round(params_.standby_width_us))});
  }
  // §19.3: every list carries its own prologue, in this order. Nothing here may
  // be skipped because "the last list already set it" -- bsl keeps no state
  // across list boundaries and has no replay to fall back on.
  emit(GalvoOp::SET_JUMP_SPEED, {params_.jump_speed_mm_s});
  emit(GalvoOp::SET_MARK_SPEED, {params_.mark_speed_mm_s});
  emit(GalvoOp::SET_LASER_DELAYS,
       {params_.laser_on_delay_us, params_.laser_off_delay_us});
  emit(GalvoOp::SET_SCANNER_DELAYS,
       {params_.scanner_mark_delay_us, params_.scanner_polygon_delay_us});
  emit(GalvoOp::SET_LASER_POWER, {params_.power_pct});
  emitPulses();
  if (params_.wobble_mode != GalvoWobbleMode::DISABLE) {
    emit(GalvoOp::SET_WOBBLE, {params_.wobble_transversal_mm,
                               params_.wobble_longitudinal_mm,
                               params_.wobble_space_mm,
                               double(int(params_.wobble_mode))});
  }
  emit(GalvoOp::ENABLE_LASER, {0});
  // A fresh list leaves the board's position unknown to us, so force the next
  // move to be absolute.
  emitted_valid_ = false;
}

void GalvoListWriter::endList() {
  if (!list_armed_) {
    return;
  }
  list_armed_ = false;
  if (!list_open_) {
    // Nothing was ever emitted: no prologue, so no epilogue either.
    return;
  }
  emit(GalvoOp::DISABLE_LASER, {0});
  // §20.1: opcode 19 is the list boundary and carries this list's estimated run
  // time in ms. This diverges from the simulator, where it takes no parameter.
  emit(GalvoOp::SET_END_OF_LIST, {list_time_ms_});
  if (gen_) {
    gen_->add_time_cost(list_time_ms_ / 1000.0);
  }
  list_open_ = false;
  list_time_ms_ = 0;
  list_command_count_ = 0;
}

void GalvoListWriter::set_mark_speed(double mm_s) {
  if (mm_s <= 0 || mm_s == params_.mark_speed_mm_s) {
    return;
  }
  params_.mark_speed_mm_s = mm_s;
  if (list_open_) {
    emit(GalvoOp::SET_MARK_SPEED, {mm_s});
  }
}

void GalvoListWriter::emitPulses() {
  // Three parameters since 2026-09-18: period us, pulse length us, Mopa pulse
  // width ns. The board clamps the third to 1..65535, so keep it in range here
  // too rather than relying on that.
  double mopa_ns = params_.mopa_pulse_ns;
  if (mopa_ns < 1) {
    mopa_ns = 1;
  } else if (mopa_ns > 65535) {
    mopa_ns = 65535;
  }
  emit(GalvoOp::SET_LASER_PULSES, {params_.pulse_period_us,
                                   params_.effective_pulse_length_us(),
                                   mopa_ns});
}

void GalvoListWriter::set_power(double pct) {
  if (pct == params_.power_pct) {
    return;
  }
  params_.power_pct = pct;
  if (list_open_) {
    emit(GalvoOp::SET_LASER_POWER, {pct});
    if (params_.derive_pulse_from_power) {
      // A CO2 head's duty cycle is its power, and opcode 11 alone does not move
      // it: the board's galvo-list path calls lcs_set_laser_power and nothing
      // else. The pulses record has to follow every power change (§19 notice).
      emitPulses();
    }
  }
}

QPointF GalvoListWriter::toField(const QPointF& p) {
  QPointF local = p - field_centre_;
  if (std::fabs(local.x()) > half_field_mm_ ||
      std::fabs(local.y()) > half_field_mm_) {
    out_of_field_count_++;
  }
  return local;
}

bool GalvoListWriter::clipSegment(QPointF& p0, QPointF& p1) const {
  // Liang-Barsky against clip_rect_.
  const double dx = p1.x() - p0.x();
  const double dy = p1.y() - p0.y();
  double t0 = 0, t1 = 1;
  const double p[4] = {-dx, dx, -dy, dy};
  const double q[4] = {p0.x() - clip_rect_.left(), clip_rect_.right() - p0.x(),
                       p0.y() - clip_rect_.top(), clip_rect_.bottom() - p0.y()};
  for (int i = 0; i < 4; i++) {
    if (p[i] == 0) {
      if (q[i] < 0) {
        return false;  // parallel to this edge and outside it
      }
      continue;
    }
    const double r = q[i] / p[i];
    if (p[i] < 0) {
      if (r > t1) return false;
      if (r > t0) t0 = r;
    } else {
      if (r < t0) return false;
      if (r < t1) t1 = r;
    }
  }
  const QPointF a(p0.x() + t0 * dx, p0.y() + t0 * dy);
  const QPointF b(p0.x() + t1 * dx, p0.y() + t1 * dy);
  p0 = a;
  p1 = b;
  return true;
}

void GalvoListWriter::jumpTo(const QPointF& p) {
  ensureListOpen();
  double dist = 0;
  if (emitted_valid_) {
    dist = QLineF(emitted_, p).length();
    if (dist == 0) {
      return;
    }
  }
  const QPointF local = toField(p);
  emit(GalvoOp::JUMP_ABS, {local.x(), local.y()});
  emitted_ = p;
  emitted_valid_ = true;
  // §14: a jump costs the traverse plus the mean of the delay-mode window.
  list_time_ms_ +=
      dist / params_.jump_speed_mm_s * 1000 + params_.jump_delay_ms();
  block_distance_mm_ += dist;
}

void GalvoListWriter::markTo(const QPointF& p) {
  ensureListOpen();
  const double dist = emitted_valid_ ? QLineF(emitted_, p).length() : 0;
  const QPointF local = toField(p);
  emit(GalvoOp::MARK_ABS, {local.x(), local.y()});
  emitted_ = p;
  emitted_valid_ = true;
  // §14: wobble stretches the path the beam actually walks.
  list_time_ms_ += dist * params_.wobble_k / params_.mark_speed_mm_s * 1000 +
                   params_.laser_delay_ms();
  block_distance_mm_ += dist;
}

void GalvoListWriter::moveTo(double x_mm, double y_mm) {
  if (!in_block_) {
    qWarning() << "GalvoListWriter::moveTo() outside a block";
    return;
  }
  QPointF p1(std::isnan(x_mm) ? cur_.x() : x_mm,
             std::isnan(y_mm) ? cur_.y() : y_mm);
  const QPointF p0 = cur_;
  cur_ = p1;
  cur_valid_ = true;

  if (!laser_on_) {
    // Travels emit nothing: the jump is deferred until something actually needs
    // the beam somewhere, which also collapses runs of travel into one jump.
    return;
  }
  QPointF a = p0;
  QPointF b = p1;
  if (clip_rect_.isValid() && !clipSegment(a, b)) {
    return;
  }
  if (params_.dotting_time_us > 0) {
    // Dot mode: opcode 5 replaces the old {23,3} + T modal pair (§4.5).
    jumpTo(b);
    ensureListOpen();
    emit(GalvoOp::LASER_ON, {params_.dotting_time_us});
    list_time_ms_ += params_.dotting_time_us / 1000.0;
    return;
  }
  if (QLineF(a, b).length() == 0) {
    return;
  }
  jumpTo(a);
  markTo(b);
  if (max_commands_per_list_ > 0 &&
      list_command_count_ >= max_commands_per_list_) {
    endList();
    beginList();
  }
}
