#include "laser.h"
#include "factory-utils.h"
#include "toolpath_exporter/toolpath-exporter-constants.h"
#include "toolpath_exporter/toolpath-utils.h"

bool LaserBitmapFactory::fg_iterate_x_pwm(const uchar* data,
                                          int l,
                                          int r,
                                          float y,
                                          bool reverse) {
  /*
  bool is_emitting = false;
  bool should_emit;
  // Note: left and right are both emitting points
  int left = left_bound;
  int right = right_bound;

  // Find the left-most emitting point
  for (; left <= right_bound; left++) {
    if (data_ptr[left] < pwm_threshold) {
      break;
    }
  }
  // Skip blank line
  if (left > right_bound) {
    return false;
  }
  // Find the right-most emitting point
  for (; right >= left; right--) {
    if (data_ptr[right] < pwm_threshold) {
      break;
    }
  }
  left = qMax(left_bound, left - padding_px_);
  right = qMin(right_bound, right + padding_px_);
  float buffer_left = getXValInMM(left, reverse_raster_dir);
  float buffer_right = getXValInMM(right + 1, reverse_raster_dir);
  moveto(layer_speed_, reverse_raster_dir ? buffer_right : buffer_left, getYValInMM(y));
  gen_->set_line_pixels(right + 1 - left);
  // 32 bits data, 8 bits per pixel (0~255)
  uint32_t current_val = 0;
  int bit_offset = 24;
  uint32_t val;

  if (reverse_raster_dir) {
    for (int x = right; x >= left; x--) {
      if (x >= left_bound && x <= right_bound && data_ptr[x] < pwm_threshold) {
        val = std::round(WHITE_PIXEL - data_ptr[x] * pwm_scale_);
      } else {
        val = 0;
      }
      current_val |= val << bit_offset;
      bit_offset -= 8;
      if (bit_offset < 0) {
        gen_->fill_32_pixels(current_val);
        current_val = 0;
        bit_offset = 24;
      }
    }
  } else {
    for (int x = left; x <= right; x++) {
      if (x >= left_bound && x <= right_bound && data_ptr[x] < pwm_threshold) {
        val = std::round(WHITE_PIXEL - data_ptr[x] * pwm_scale_);
      } else {
        val = 0;
      }
      current_val |= val << bit_offset;
      bit_offset -= 8;
      if (bit_offset < 0) {
        gen_->fill_32_pixels(current_val);
        current_val = 0;
        bit_offset = 24;
      }
    }
  }
  if (bit_offset != 24) {
    gen_->fill_32_pixels(current_val);
  }
  gen_->set_fill_end();
  gen_->set_print_line_status();
  moveto(NAN, reverse_raster_dir ? buffer_left : buffer_right);
  */
  return true;
}

bool LaserBitmapFactory::fg_iterate_x(const uchar* data,
                                      int l,
                                      int r,
                                      float y,
                                      bool reverse) {
  /*
  bool is_emitting = false;
  bool should_emit;
  // Note: left and right are both emitting points
  int left = left_bound;
  int right = right_bound;

  // Find the left-most emitting point
  for (; left <= right_bound; left++) {
    if (data_ptr[left] < WHITE_PIXEL) {
      break;
    }
  }
  // Skip blank line
  if (left > right_bound) {
    return false;
  }
  // Find the right-most emitting point
  for (; right >= left; right--) {
    if (data_ptr[right] < WHITE_PIXEL) {
      break;
    }
  }
  left = qMax(left_bound, left - padding_px_);
  right = qMin(right_bound, right + padding_px_);
  float buffer_left = getXValInMM(left, reverse_raster_dir);
  float buffer_right = getXValInMM(right + 1, reverse_raster_dir);
  moveto(layer_speed_ + 1, reverse_raster_dir ? buffer_right : buffer_left, getYValInMM(y));
  moveto(layer_speed_);
  gen_->set_line_pixels(right + 1 - left);
  // 32 bits data, 1 bit per pixel (0 or 1)
  uint32_t current_val = 0;
  int bit_id = 31;

  if (reverse_raster_dir) {
    for (int x = right; x >= left; x--) {
      if (data_ptr[x] < WHITE_PIXEL) {
        current_val |= 1 << bit_id;
      }
      bit_id -= 1;
      if (bit_id < 0) {
        gen_->fill_32_pixels(current_val);
        current_val = 0;
        bit_id = 31;
      }
    }
  } else {
    for (int x = left; x <= right; x++) {
      if (data_ptr[x] < WHITE_PIXEL) {
        current_val |= 1 << bit_id;
      }
      bit_id -= 1;
      if (bit_id < 0) {
        gen_->fill_32_pixels(current_val);
        current_val = 0;
        bit_id = 31;
      }
    }
  }
  if (bit_id != 31) {
    gen_->fill_32_pixels(current_val);
  }
  gen_->set_fill_end();
  gen_->set_print_line_status();
  moveto(NAN, reverse_raster_dir ? buffer_left : buffer_right);
  */
  return true;
}

bool LaserBitmapFactory::iterate_x(const uchar* data,
                                   int l,
                                   int r,
                                   float y,
                                   bool reverse) {
  /*
  bool is_emitting = false;
  // Last meaningful point(non-emitting after emitting or every emitting points)
  int current_x = -1;
  int x = reverse_raster_dir ? right_bound : left_bound;
  int x_step = reverse_raster_dir ? -1 : 1;
  // Merge consecutive emitting points
  // Handle move on next non-emitting point(or end of line)
  bool has_unfinished_move = false;

  while (true) {
    if (reverse_raster_dir) {
      if (x < left_bound) {
        break;
      }
    } else if (x > right_bound) {
      break;
    }

    if (data_ptr[x] < WHITE_PIXEL) {
      if (is_emitting) {
        // Consecutive emitting points
        current_x = x;
        has_unfinished_move = true;
      } else {
        // Laser off -> on
        if (current_x == -1) {
          // First emitting point of this line; should handle y movement
          gen_->set_toolhead_pwm(0);
          // And by adding speed+1 hack, machine will refresh the speed value
          moveto(layer_speed_ + 1, NAN, getYValInMM(y));
          moveto(layer_speed_);
        }
        current_x = x;
        moveto(NAN,
               getXValInMM(reverse_raster_dir ? current_x + 1 : current_x,
                           reverse_raster_dir, true));
        has_unfinished_move = false;
        gen_->set_toolhead_pwm(100);
        is_emitting = true;
      }
    } else if (is_emitting) {
      // Laser on -> off
      current_x = x;
      moveto(NAN,
             getXValInMM(reverse_raster_dir ? current_x + 1 : current_x,
                         reverse_raster_dir, true));
      has_unfinished_move = false;
      gen_->set_toolhead_pwm(0);
      is_emitting = false;
    }

    x += x_step;
  }

  if (current_x >= 0) {
    qreal real_x = px2mm(reverse_raster_dir ? current_x + 1 : current_x, true);
    if (has_unfinished_move) {
      qreal move_x = qMax(real_x, float(0)) - module_offset_.x();
      moveto(NAN, move_x);
    }
    if (!reverse_raster_dir)
      real_x += backlash_;
    qreal laser_padding = 25;
    if (config_.enable_mock_fast_gradient) {
      laser_padding = padding_mm_;
    }
    qreal buffer_x;
    // Check boundary without module offset (may include job origin)
    if (reverse_raster_dir) {
      buffer_x = qMax(real_x - laser_padding, 0.0);
    } else {
      buffer_x = qMin(real_x + laser_padding, work_area_mm_.width());
    }
    buffer_x -= module_offset_.x();
    gen_->set_toolhead_pwm(0);
    moveto(NAN, buffer_x);
    return true;
  } else {
    // Blank line
    return false;
  }
  */
  return true;
}
