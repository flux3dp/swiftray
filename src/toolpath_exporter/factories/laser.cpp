#include "laser.h"
#include "factory-utils.h"
#include "toolpath_exporter/toolpath-exporter-constants.h"
#include "toolpath_exporter/toolpath-utils.h"

LaserBitmapFactory::LaserBitmapFactory(const FactoryKwargs& kwargs) noexcept
    : BaseBitmapFactory(kwargs), fg_pwm_limit(kwargs.fg_pwm_limit) {
  qInfo() << "LaserBitmapFactory created";
}

void LaserBitmapFactory::set_default_workspace(int val) {
  default_workspaces_index = val;
}

void LaserBitmapFactory::set_pwm_engraving(bool val) {
  pwm_engraving = val;
}

void LaserBitmapFactory::add_filled_path(QPainterPath& path,
                                         QRectF& bbox,
                                         int index) {
  if (index < 0) {
    index = default_workspaces_index;
  }
  get_workspace(index, true)->add_filled_path(path, bbox);
}

// FIXME: padding arg not used
// Whole-raster iteration boxes (non-block path). Block-split tasks instead use
// the exporter's blocks via set_blocks()/generate_task_code().
QVector<QRect> LaserBitmapFactory::get_iteration_data(int padding_pixel,
                                                      bool reverse_y) {
  auto workspace = get_workspace();
  QVector<QRect> boxes =
      get_bounding_boxes(workspace->get_bitmap(), workspace->get_dirty_area(),
                         padding_px, padding_px, 5, split_bbox, pixel_per_mm / 5);
  qInfo() << "boxes" << boxes;
  // Ordering (previously done by the caller): engrave from the far side first
  // when !reverse_y. This reverses only the Y (band) order.
  qInfo() << "reverse_y" << reverse_y;
  if (!reverse_y) {
    std::reverse(boxes.begin(), boxes.end());
  }
  return boxes;
}

QRect LaserBitmapFactory::block_region_to_px(const QRectF& region_mm) const {
  // Round both edges the same way so that adjacent blocks (which share a mm
  // border) tile the pixel grid with no gap and no overlap.
  int px_l = std::lround(region_mm.left() * pixel_per_mm_x);
  int px_r = std::lround(region_mm.right() * pixel_per_mm_x);
  int px_t = std::lround(region_mm.top() * pixel_per_mm);
  int px_b = std::lround(region_mm.bottom() * pixel_per_mm);
  return QRect(px_l, px_t, px_r - px_l, px_b - px_t);
}

void LaserBitmapFactory::iterate_region(const QRect& region,
                                        bool reverse_y,
                                        ScanMethod method,
                                        QImage* src_bitmap,
                                        int pass) {
  if (!region.isValid()) {
    return;
  }
  // Cross-pass: alternate the serpentine phase per pass so runs reverse
  // direction between passes (residual lead-in deficits land on opposite ends).
  bool reverse_x = (pass & 1) != 0;
  int x = region.x(), y = region.y(), w = region.width(), h = region.height();
  int step = pwm_engraving && fg_pwm_limit
                 ? std::max(fg_pwm_limit - padding_px * 2, 100)
                 : w;
  for (int left = x; left < x + w; left += step) {
    int right = std::min(left + step, x + w);
    for (int yy = 0; yy < h; yy++) {
      int i = reverse_y ? (y + h - 1 - yy) : (y + yy);
      double real_y = pixel_to_actual_position(left, i).y();
      real_y = std::round((real_y - offset.y()) * 100) / 100.0;
      bool engraved = (this->*method)(src_bitmap->constScanLine(i),
                                      left,   // inclusive
                                      right,  // exclusive
                                      real_y, reverse_x);
      if (engraved && !one_way) {
        reverse_x = !reverse_x;
      }
    }
  }
}

void LaserBitmapFactory::generate_task_code(GenerateTaskKwargs kwargs) {
  if (!is_workspace_valid()) {
    return;
  }
  auto workspace = get_workspace();

  if (kwargs.support_fast_gradient) {
    if (pixel_per_mm_x == 5) {
      proc->turn_on_gradient_print_mode(pwm_engraving ? 'P' : 'L');
    } else if (pixel_per_mm_x == 10) {
      proc->turn_on_gradient_print_mode(pwm_engraving ? 'Q' : 'M');
    } else if (pixel_per_mm_x == 20) {
      proc->turn_on_gradient_print_mode(pwm_engraving ? 'R' : 'H');
    } else if (pixel_per_mm_x == 39) {
      proc->turn_on_gradient_print_mode(pwm_engraving ? 'C' : 'B');
    } else if (pixel_per_mm_x == 40) {
      proc->turn_on_gradient_print_mode(pwm_engraving ? 'S' : 'U');
    } else if (pixel_per_mm_x == 78) {
      proc->turn_on_gradient_print_mode(pwm_engraving ? 'T' : 'A');
    } else {
      qWarning() << "Unsupported pixel_per_mm_x for fast gradient:"
                 << pixel_per_mm_x;
    }
  }

  speed = kwargs.speed;
  backlash = kwargs.backlash;
  pwm_scale = kwargs.pwm_scale;
  mock_fast_gradient = kwargs.mock_fast_gradient;
  padding_dist = kwargs.padding_dist;
  padding_px = get_padding_pixels(padding_dist);

  ScanMethod method = nullptr;
  if (kwargs.support_fast_gradient) {
    if (pwm_engraving) {
      method = &LaserBitmapFactory::fg_iterate_x_pwm;
    } else {
      method = &LaserBitmapFactory::fg_iterate_x;
    }
  } else {
    method = &LaserBitmapFactory::iterate_x;
  }
  QImage* src_bitmap = workspace->get_bitmap();

  // Dev fluence handler runs on the plain (non-fast-gradient) raster path only.
  fluence_.set_run_params(fluence_power_pct_, speed);
  // Cross-pass repeats each block N times with alternating run direction; the
  // tube envelope is tracked continuously across the passes (reset per block).
  const int passes = fluence_.active() ? fluence_.cross_pass() : 1;
  auto emit_region = [&](const QRect& region) {
    fluence_.begin_block();  // reset the tube envelope per block/tile
    for (int pass = 0; pass < passes; pass++) {
      iterate_region(region, kwargs.reverse_y, method, src_bitmap, pass);
    }
  };

  if (!block_regions_mm_.isEmpty()) {
    padding_dist = 0;
    padding_px = 0;
    // Block-split task: emit one block at a time using the exporter's blocks
    // (the same blocks_ used for paths, or a single block when the layer fits
    // the galvo field). Only blocks that overlap the dirty area are emitted,
    // each wrapped by the start/end callbacks (enter/exit Promark mode).
    QRect dirty = workspace->get_dirty_area().toAlignedRect();
    for (int bi = 0; bi < block_regions_mm_.size(); bi++) {
      QRect region = block_region_to_px(block_regions_mm_[bi]).intersected(dirty);
      if (region.isEmpty()) {
        continue;  // no content in this block
      }
      if (block_start_cb_) {
        block_start_cb_(bi);
      }
      emit_region(region);
      if (block_end_cb_) {
        block_end_cb_();
      }
    }
  } else {
    // Whole-raster task: get_iteration_data() applies far-side-first ordering.
    QVector<QRect> bboxes = get_iteration_data(padding_px, kwargs.reverse_y);
    for (const QRect& bbox : bboxes) {
      emit_region(bbox);
    }
  }
  if (kwargs.support_fast_gradient) {
    proc->turn_off_gradient_print_mode();
  }
}

bool LaserBitmapFactory::fg_iterate_x_pwm(const uchar* data,
                                          int l,
                                          int r,
                                          float y,
                                          bool reverse) {
  int first_emit_index = l;     // first_zero_index
  int last_emit_index = r - 1;  // last_zero_index, inclusive

  // Find the left-most emitting point
  for (; first_emit_index < r; first_emit_index++) {
    if (data[first_emit_index] < pwm_threshold) {
      break;
    }
  }
  // Skip blank line
  if (first_emit_index >= r) {
    return false;
  }
  // Find the right-most emitting point
  for (; last_emit_index > first_emit_index; last_emit_index--) {
    if (data[last_emit_index] < pwm_threshold) {
      break;
    }
  }
  // PWM slice may not contain full padding area when cut by fg_pwm_limit, so
  // check again here
  int left = qMax(0, first_emit_index - padding_px);
  int right =
      qMin(work_area.width() - 1, last_emit_index + padding_px);  // inclusive
  int pixel_number = right - left + 1;
  float left_x = pixel_size_x * left - offset.x();
  float right_x = pixel_size_x * (right + 1) - offset.x();
  if (!reverse) {
    left_x += backlash;
    right_x += backlash;
  }
  proc->moveto(NamedArgs().rx(reverse ? right_x : left_x).ry(y).set_is_travel());
  proc->moveto(NamedArgs().rx(reverse ? right_x : left_x).ry(y));  // for 3d curve, move z to start position
  proc->set_line_pixels(pixel_number);

  // 32 bits data, 8 bits per pixel (0~255)
  uint32_t current_val = 0;
  int bit_offset = 24;
  uint32_t point;
  int i, idx;
  for (int i = left; i <= right; i++) {
    idx = reverse ? (right - i + left) : i;
    if (idx >= l && idx < r && data[idx] < pwm_threshold) {
      point = std::round(WHITE_PIXEL - data[idx] * pwm_scale);
    } else {
      point = 0;
    }
    current_val |= point << bit_offset;
    bit_offset -= 8;
    if (bit_offset < 0) {
      proc->fill_32_pixels(current_val);
      current_val = 0;
      bit_offset = 24;
    }
  }
  if (bit_offset != 24) {
    proc->fill_32_pixels(current_val);
  }
  proc->set_fill_end();
  proc->set_print_line_status();
  proc->moveto(NamedArgs().rx(reverse ? left_x : right_x).rf(speed));
  return true;
}

bool LaserBitmapFactory::fg_iterate_x(const uchar* data,
                                      int l,
                                      int r,
                                      float y,
                                      bool reverse) {
  int first_emit_index = l;
  int last_emit_index = r - 1;

  // Find the left-most emitting point
  for (; first_emit_index < r; first_emit_index++) {
    if (data[first_emit_index] < WHITE_PIXEL) {
      break;
    }
  }
  // Skip blank line
  if (first_emit_index >= r) {
    return false;
  }
  // Find the right-most emitting point
  for (; last_emit_index > first_emit_index; last_emit_index--) {
    if (data[last_emit_index] < WHITE_PIXEL) {
      break;
    }
  }
  int left = qMax(l, first_emit_index - padding_px);
  int right = qMin(r - 1, last_emit_index + padding_px);
  int pixel_number = right - left + 1;
  float left_x = pixel_size_x * left - offset.x();
  float right_x = pixel_size_x * (right + 1) - offset.x();
  if (!reverse) {
    left_x += backlash;
    right_x += backlash;
  }
  proc->moveto(NamedArgs().rx(reverse ? right_x : left_x).ry(y).set_is_travel());
  proc->moveto(NamedArgs().rx(reverse ? right_x : left_x).ry(y));  // for 3d curve, move z to start position
  proc->set_line_pixels(pixel_number);

  // 32 bits data, 1 bit per pixel (0 or 1)
  uint32_t current_val = 0;
  int bit_id = 31;
  int i, idx;
  for (int i = left; i <= right; i++) {
    idx = reverse ? (right - i + left) : i;
    if (data[idx] < WHITE_PIXEL) {
      current_val |= 1 << bit_id;
    }
    bit_id -= 1;
    if (bit_id < 0) {
      proc->fill_32_pixels(current_val);
      current_val = 0;
      bit_id = 31;
    }
  }
  if (bit_id != 31) {
    proc->fill_32_pixels(current_val);
  }
  proc->set_fill_end();
  proc->set_print_line_status();
  proc->moveto(NamedArgs().rx(reverse ? left_x : right_x).rf(speed));
  return true;
}

bool LaserBitmapFactory::iterate_x_fluence(const uchar* data,
                                           int l,
                                           int r,
                                           float y,
                                           bool reverse) {
  // Detect contiguous dark pixel runs and hand each to the fluence emitter,
  // which owns laser gating + ramp-compensating speed segmentation. The emitter
  // works in machine mm, so convert pixel edges the same way iterate_x does
  // (clamp >=0, subtract offset, add backlash on forward scans).
  auto edge_to_machine = [&](int edge_px) -> double {
    float v = pixel_size_x * edge_px;
    v = qMax(v, 0.0f) - offset.x();
    if (!reverse) {
      v += backlash;
    }
    return v;
  };
  QVector<QPair<int, int>> spans;  // inclusive [c0, c1] dark column ranges
  int start = -1;
  for (int c = l; c < r; c++) {
    if (data[c] < WHITE_PIXEL) {
      if (start < 0) start = c;
    } else if (start >= 0) {
      spans.append({start, c - 1});
      start = -1;
    }
  }
  if (start >= 0) spans.append({start, r - 1});
  if (spans.isEmpty()) {
    return false;
  }
  if (reverse) std::reverse(spans.begin(), spans.end());
  for (const QPair<int, int>& s : spans) {
    // Run spans the outer edges of the dark pixel range, in traverse direction.
    double left = edge_to_machine(s.first);
    double right = edge_to_machine(s.second + 1);
    if (reverse) {
      fluence_.emit_run(QPointF(right, y), QPointF(left, y));
    } else {
      fluence_.emit_run(QPointF(left, y), QPointF(right, y));
    }
  }
  return true;
}

bool LaserBitmapFactory::iterate_x(const uchar* data,
                                   int l,
                                   int r,
                                   float y,
                                   bool reverse) {
  if (fluence_.active() && !mock_fast_gradient) {
    return iterate_x_fluence(data, l, r, y, reverse);
  }
  bool is_emitting = false;      // current_laser_val
  bool should_emitting = false;  // laser_value
  bool has_moved_x = false, has_moved_y = false;
  int current_x = -1;
  int i, x;
  float real_x;
  for (int i = l; i < r; i++) {
    x = reverse ? (r - 1 - i + l) : i;
    should_emitting = data[x] < WHITE_PIXEL;

    if (!should_emitting) {
      if (is_emitting) {
        // Laser on -> off
        is_emitting = false;
        current_x = x;
        real_x = pixel_size_x * (reverse ? (x + 1) : x);
        real_x = qMax(real_x, 0.0f) - offset.x();
        if (!reverse) {
          real_x += backlash;
        }
        proc->moveto(NamedArgs().rx(real_x));
        proc->set_toolhead_pwm(0);
      }
    } else {
      if (!is_emitting) {
        // Laser off -> on
        if (!has_moved_y) {
          // First emitting point of this line; should handle y movement
          proc->set_toolhead_pwm(0);
          proc->moveto(NamedArgs().ry(y).set_is_travel());
          proc->moveto(NamedArgs().rf(speed));
          has_moved_y = true;
        }
        is_emitting = true;
        current_x = x;
        has_moved_x = true;

        real_x = pixel_size_x * (reverse ? (x + 1) : x);
        real_x = qMax(real_x, 0.0f) - offset.x();
        if (!reverse) {
          real_x += backlash;
        }
        proc->moveto(NamedArgs().rx(real_x));
        proc->set_toolhead_pwm(100);
      } else {
        // Consecutive emitting points
        current_x = x;
        has_moved_x = false;
      }
    }
  }

  if (current_x >= 0) {
    real_x = pixel_size_x * (reverse ? (current_x + 1) : current_x);
    float move_x = qMax(real_x, 0.0f) - offset.x();
    if (!reverse) {
      real_x += backlash;
    }
    if (!has_moved_x) {
      proc->moveto(NamedArgs().rx(move_x));
    }
    float laser_padding = mock_fast_gradient ? padding_dist : 25;
    float buffer_x;
    if (!reverse) {
      buffer_x = qMin(real_x + laser_padding, work_area_mm.width()) - offset.x();
    } else {
      buffer_x = qMax(real_x - laser_padding, 0.0f) - offset.x();
    }
    proc->set_toolhead_pwm(0);
    proc->moveto(NamedArgs().rx(buffer_x));
  }
  return current_x >= 0;
}
