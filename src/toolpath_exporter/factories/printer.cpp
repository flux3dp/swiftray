#include "printer.h"
#include "factory-utils.h"
#include "toolpath_exporter/toolpath-utils.h"
#include "toolpath_exporter/toolpath-exporter-constants.h"

PrinterBitmapFactory::PrinterBitmapFactory(const FactoryKwargs& kwargs,
                                           int slice_width,
                                           int slice_height,
                                           int slice_top_padding,
                                           int slice_bot_padding) noexcept
    : BaseBitmapFactory(std::move(normalize(kwargs))),
      interpolation(kwargs.interpolation),
      halftone(kwargs.halftone),
      color_curve(kwargs.color_curve) {
  qInfo() << "PrinterBitmapFactory created";
  if (kwargs.halftone_params) {
    halftone_params = *(kwargs.halftone_params);
  }
  this->slice_width = slice_width * interpolation;
  this->max_slice_width = slice_width;
  this->slice_height = slice_height * interpolation;
  this->max_slice_height = slice_height;
  this->slice_top_padding = slice_top_padding * interpolation;
  this->slice_bot_padding = slice_bot_padding * interpolation;
}

void PrinterBitmapFactory::set_slice_width(int val) {
  if (val > 0 && val <= max_slice_width)
    slice_width = val;
}

void PrinterBitmapFactory::set_slice_height(int val) {
  if (val > 0 && val <= max_slice_height)
    slice_height = val;
}

void PrinterBitmapFactory::set_slice_top_padding(int val) {
  if (val >= 0)
    slice_top_padding = val;
  else
    slice_top_padding = SLICE_TOP_PADDING;
}

void PrinterBitmapFactory::set_slice_bot_padding(int val) {
  if (val >= 0)
    slice_bot_padding = val;
  else
    slice_bot_padding = SLICE_BOT_PADDING;
}

void PrinterBitmapFactory::set_nozzle_mode(int val) {
  if (val < 1 || val > 3) {
    return;
  }
  nozzle_mode_ = NozzleMode(val);
}

void PrinterBitmapFactory::set_nozzle_offset(NozzleMode nozzle,
                                             const QPointF& nozzle_offset) {
  nozzle_offsets[nozzle] = nozzle_offset;
}

QPointF PrinterBitmapFactory::pixel_to_actual_position(int x,
                                                       int y,
                                                       NozzleMode nozzle_mode,
                                                       bool apply_backlash) {
  QPointF real_pos = BaseBitmapFactory::pixel_to_actual_position(x, y);
  real_pos -= offset;
  if (nozzle_mode != NozzleMode::UNDEFINED &&
      nozzle_offsets.contains(nozzle_mode)) {
    auto nozzle_offset = nozzle_offsets[nozzle_mode];
    real_pos -= nozzle_offset;
  }
  if (apply_backlash && backlash > 0) {
    real_pos.rx() -= backlash;
  }
  return real_pos;
}

void PrinterBitmapFactory::setup_clip_rect(
    const std::shared_ptr<Workspace>& workspace) {
  if (clip_rect.isNull()) {
    return;
  }
  int bottom = clip_rect.bottom();
  int real_bottom = qMin(bottom + slice_height, work_area.height());
  QRect real_clip_rect = QRect(clip_rect);
  real_clip_rect.setBottom(real_bottom);
  workspace->set_clip_rect(real_clip_rect);
}

void PrinterBitmapFactory::generate_image_for_task(double black_ratio) {
  auto workspace = get_workspace();
  bitmap_dirty_area = workspace->get_dirty_area();
  int bbox_left = bitmap_dirty_area.left();
  int bbox_top = bitmap_dirty_area.top();
  int bbox_right = bitmap_dirty_area.right();
  int bbox_bottom = bitmap_dirty_area.bottom();
  if (bbox_left > bbox_right || bbox_top > bbox_bottom) {
    return;
  }

  bool do_color_curve = !color_curve.isEmpty();
  bool do_am = halftone > 1;

  double smoother = halftone_params.smoother;
  double multiplier = halftone_params.multiplier * black_ratio;
  double am_cos, am_sin, am_dot_r, am_dot_d;
  if (do_am) {
    double rad = halftone_params.angle * M_PI / 180;
    am_cos = cos(rad);
    am_sin = sin(rad);
    am_dot_r = qMax(pixel_per_mm / (2.0 * halftone_params.density), 1.0);
    am_dot_d = am_dot_r * 2;
  }

  QImage src_bitmap = workspace->bitmap;
  bitmap = QImage(work_area, QImage::Format_Grayscale8);
  bitmap.fill(Qt::white);
  // Preprocess image
  for (int y = bbox_top; y <= bbox_bottom; y++) {
    uchar* data_ptr = bitmap.scanLine(y);
    uchar* src_data_ptr = src_bitmap.scanLine(y);
    for (int x = bbox_left; x <= bbox_right; x++) {
      int inv_val = WHITE_PIXEL - src_data_ptr[x];
      if (inv_val == 0) {
        // Skip white pixels
        continue;
      }
      // Color curve
      if (do_color_curve) {
        inv_val = apply_color_curve(inv_val, color_curve);
      }
      // Halftone
      if (do_am) {
        inv_val = am_halftone(inv_val, x, y, am_cos, am_sin, am_dot_r, am_dot_d,
                              smoother, multiplier);
      } else {
        inv_val = fm_halftone(inv_val, smoother, multiplier);
      }
      data_ptr[x] = (uchar)(WHITE_PIXEL - inv_val);
    }
  }
  if (!do_am) {
    bitmap.convertTo(QImage::Format_Mono, Qt::DiffuseDither);
    bitmap.convertTo(QImage::Format_Grayscale8);
  }
}

void PrinterBitmapFactory::generate_task_code(GenerateTaskKwargs kwargs) {
  if (kwargs.repeat == 0 || workspaces->isEmpty()) {
    return;
  }
  double min_padding_left = qMax(kwargs.min_padding_left, kwargs.min_padding);
  double min_padding_right = qMax(kwargs.min_padding_right, kwargs.min_padding);
  int padding_left = get_padding_pixels(
      get_padding_dist(min_padding_left, kwargs.speed / 60, kwargs.acc));
  int padding_right = get_padding_pixels(
      get_padding_dist(min_padding_right, kwargs.speed / 60, kwargs.acc));
  generate_image_for_task(kwargs.black_ratio);
  prepare_table();

  int total_block = 0;
  QVector<QRect> contour_boxes =
      get_bounding_boxes(&bitmap, bitmap_dirty_area, padding_left,
                         padding_right, slice_height, split_bbox);
  QVector<BlockBoxes> blocks = {};
  for (auto box : contour_boxes) {
    blocks.append(slice_image(box, kwargs.reverse_y, kwargs.multipass));
    preprocess_box_data(box);
    total_block += blocks.constLast().size();
  }
  if (this->cancelled)
    return;
  onProgressChanged(0.05, true);

  // Generate fcode
  NamedArgs args_s0 = NamedArgs().rs(0);
  float progress_unit = 0.9 / total_block / kwargs.repeat;
  bool reverse_x = false;
  bool no_cache = true;
  for (int r = 0; r < kwargs.repeat; r++) {
    for (BlockBoxes& block : blocks) {
      reverse_x = false;
      for (RowBoxes& row : block) {
        int row_size = row.size();
        for (int i = 0; i < row_size; i++) {
          int real_i = reverse_x ? (row_size - 1 - i) : i;
          SlicedBox bbox = row[real_i];
          if (no_cache) {
            bbox.data.append(create_image_packet_data(
                bbox, padding_left, padding_right, reverse_x));
          }
          PacketData data = bbox.data[0];
          if (data.px_count == 0) {
            continue;
          }
          write_data_to_proc(data.box,
                             data.payload,
                             data.px_count, kwargs.speed, reverse_x);
        }
        proc->moveto(args_s0);
        if (!one_way) {
          reverse_x = !reverse_x;
        }
      }
    }
    no_cache = false;
  }
  if (this->cancelled)
    return;
  onProgressChanged(0.95, true);
}

void PrinterBitmapFactory::prepare_table() {
  val_table = QImage{bitmap.size(), QImage::Format_Grayscale8};
  val_table.fill(0);
}

BlockBoxes PrinterBitmapFactory::slice_image(QRect box,
                                             bool reverse_y,
                                             int multipass) {
  int box_left = box.x();
  int box_top = box.y();
  int box_right = box_left + box.width();
  int box_bottom = box_top + box.height();

  double offset_y_px = offset.y() * pixel_per_mm;
  double min_allow_y = qMax(offset_y_px, 0.0);
  double bottom_limit =
      qMax(-offset_y_px, 0.0) + clip_rect.bottom() * pixel_per_mm;
  int max_allow_y = bottom_limit - (interpolation - 1);
  int block_height = slice_height;
  int print_height = slice_height - slice_top_padding - slice_bot_padding;

  int box_w;
  QVector<SlicedBox> x_boxes;  // boxes with only x direction info
  int x_step = slice_width == 0 ? box.width() : slice_width;
  for (int box_x = box_left; box_x < box_right; box_x += x_step) {
    box_w = qMin(x_step, box_right - box_x);
    x_boxes.append(SlicedBox(box_x, 0, box_w, 0));
  }

  int box_y, box_h, padding_top, shift;
  QVector<SlicedBox> y_boxes;  // boxes with only y direction info
  for (int p = multipass - 1; p >= 0; p--) {
    int padding = ((p * print_height) / multipass) + slice_top_padding;
    int y = box_top - padding;
    for (int j = y; j < box_bottom; j += print_height) {
      if (j + slice_top_padding > box_bottom) {
        break;
      }
      if (j < min_allow_y) {
        box_y = std::ceil(min_allow_y);
        shift = box_y - j;
        box_h = print_height + slice_top_padding - shift;
        padding_top = qMax(slice_top_padding - shift, 0);
      } else if (j > max_allow_y) {
        box_y = max_allow_y;
        padding_top = slice_top_padding + j - max_allow_y;
        if (padding_top >= block_height) {
          continue;
        }
        box_h = print_height + padding_top;
      } else {
        box_y = j;
        padding_top = slice_top_padding;
        box_h = print_height + padding_top;
      }
      box_h = qMin(qMin(box_h, box_bottom - box_y), block_height);
      for (int interpolation_offset = 0; interpolation_offset < interpolation;
           interpolation_offset++) {
        y_boxes.append(
            SlicedBox(0, box_y + interpolation_offset, 0, box_h, padding_top));
      }
    }
  }
  std::sort(y_boxes.begin(), y_boxes.end(),
            [reverse_y](const SlicedBox& a, const SlicedBox& b) {
              if (a.y() != b.y())
                return a.y() > b.y() == reverse_y;
              return a.height() > b.height() == reverse_y;
            });

  BlockBoxes boxes;  // final boxes with full info
  for (auto& y_box : y_boxes) {
    RowBoxes row_boxes;
    for (auto& x_box : x_boxes) {
      row_boxes.append(SlicedBox(x_box.x(), y_box.y(), x_box.width(),
                                 y_box.height(), y_box.padding_top));
    }
    boxes.append(row_boxes);
  }
  return boxes;
}

void PrinterBitmapFactory::set_preparatory_task_bbox(QRectF bbox) {
  int x = std::round(bbox.x() * pixel_per_mm);
  int y = std::round(bbox.y() * pixel_per_mm);
  int w = std::round(bbox.width() * pixel_per_mm);
  int h = std::round(bbox.height() * pixel_per_mm);
  int prespray_width_px = int(prespray_width * pixel_per_mm);
  int x_safe_dist = std::round(prespray_safe_x * pixel_per_mm);
  int prespray_x, prespray_y, prespray_w, prespray_h;
  int test_x, test_y, test_w, test_h;
  if (w > prespray_width_px + 2 * x_safe_dist) {
    prespray_x = x + (w - prespray_width_px) / 2;
    prespray_w = prespray_width_px;
  } else {
    prespray_x = x + x_safe_dist;
    prespray_w = w - 2 * x_safe_dist;
  }
  test_x = x + x_safe_dist;
  test_w = w - 2 * x_safe_dist;
  if (h > 2 * slice_height) {
    int padding = (h - 2 * slice_height) / 3;
    prespray_y = y + padding;
    prespray_h = slice_height;
    test_y = y + 2 * padding + slice_height;
    test_h = slice_height;
  } else if (h > slice_height) {
    int padding = (h - slice_height) / 2;
    prespray_y = y + padding;
    prespray_h = slice_height;
    test_y = y + padding;
    test_h = slice_height;
  } else {
    prespray_y = y;
    prespray_h = h;
    test_y = y;
    test_h = h;
  }
  prespray_bbox = QRect(prespray_x, prespray_y, prespray_w, prespray_h);
  cartridge_test_bbox = QRect(test_x, test_y, test_w, test_h);
}

void PrinterBitmapFactory::generate_solid_block(QRect box,
                                                float speed,
                                                bool reverse_x,
                                                NozzleMode nozzle_mode,
                                                int multipass) {
  int x = box.x();
  int y = box.y();
  int w = box.width();
  int h = box.height();
  int pass_interval = slice_height / multipass;
  int pass_y = y - pass_interval * (multipass - 1);
  int px_count, bit_count, init_idx, bit_idx, cur_val, pixel_value;
  while (pass_y < y + h) {
    px_count = 0;
    QByteArray payload;
    payload.append((const char*)(&w), 4);
    payload.append((const char*)(&slice_height), 4);
    payload.append(8, (char)0);  // x, y
    if (!is_4c) {
      payload.append(4, (char)0);  // reserved
    }
    bit_count = is_4c ? 4 : 1;
    init_idx = 8 - bit_count;
    for (int i = 0; i < w; i++) {
      bit_idx = init_idx;
      cur_val = 0;
      pixel_value = (1 << bit_count) - 1;
      for (int j = 0; j < slice_height; j += 1) {
        // don't know why but the y range should be reversed for single color
        int r = is_4c ? pass_y + j : pass_y + slice_height - 1 - j;
        if (r < y + h && r >= y) {
          cur_val += pixel_value << bit_idx;
          px_count += bit_count;
        }
        bit_idx -= bit_count;
        if (bit_idx < 0) {
          payload.append(cur_val);
          bit_idx = init_idx;
          cur_val = 0;
        }
      }
      if (bit_idx != init_idx) {
        payload.append(cur_val);
      }
    }
    write_data_to_proc(SlicedBox(x, pass_y, w, h, 0), payload, px_count, speed,
                       reverse_x, true, nozzle_mode);
    pass_y += pass_interval;
  }
}

void PrinterBitmapFactory::generate_prespray_task_code(float speed,
                                                       bool reverse_x,
                                                       NozzleMode nozzle_mode) {
  if (prespray_bbox.isNull()) {
    return;
  }
  generate_solid_block(prespray_bbox, speed, reverse_x, nozzle_mode);
}

void PrinterBitmapFactory::generate_cartridge_task_code(
    int multipass,
    float speed,
    bool reverse_x,
    NozzleMode nozzle_mode) {
  if (cartridge_test_bbox.isNull()) {
    return;
  }
  generate_solid_block(cartridge_test_bbox, speed, reverse_x, nozzle_mode,
                       multipass);
}

void PrinterBitmapFactory::preprocess_box_data(const QRect& box) {
  int x = box.x();
  int y = qMax(box.y() - slice_height - 1, 0);
  int w = box.width();
  int h = qMin(box.height() + slice_height * 2, bitmap.height() - y);
  int right = x + w;   // exclusive
  int bottom = y + h;  // exclusive

  const uchar* last_val_ptr = y > 0 ? val_table.constScanLine(y - 1) : nullptr;
  for (int r = y; r <= bottom; r++) {
    const uchar* data_ptr = bitmap.constScanLine(r);
    uchar* val_ptr = val_table.scanLine(r);
    for (int c = x; c < right; c++) {
      uchar val = data_ptr[c] == WHITE_PIXEL ? 0 : 0b10000000;
      uchar prev_val = last_val_ptr ? last_val_ptr[c] : 0;
      if ((prev_val | val) == 0) {
        // All 9 involving pixels are white; keep val = 0
        continue;
      }
      val_ptr[c] = prev_val >> 1 | val;
    }
    last_val_ptr = val_ptr;
  }
}

// For normal printing packet without x_step
PacketData PrinterBitmapFactory::create_image_packet_data(
    const SlicedBox& box,
    int padding_left,
    int padding_right,
    bool reverse_x) {
  int x = box.x();
  int y = box.y();
  int w = box.width();
  int h = box.height();
  int min_data_idx = -1;
  int max_data_idx = -1;
  int max_y = y + h;  // excluded
  int min_y = y + box.padding_top;  // included

  int max_x = x + w;  // excluded
  int padded_w = -1;

  QVector<QByteArray> payload_data;
  int px_count = 0;
  int c;
  for (int i = 0; i < w; i++) {
    c = reverse_x ? (x + w - 1 - i) : (x + i);
    int column_count = 0;
    QByteArray column_payload;
    for (int r = y + slice_height - 1; r >= y; r -= 8) {
      if (r > max_y + 8 || r < 0 || r < min_y) {
        column_payload.append((char)0);
      } else if (r >= max_y) {
        int overflow = r - max_y + 1;
        uchar val = val_table.constScanLine(max_y - 1)[c] >> overflow;
        column_payload.append(val);
        column_count += ((std::bitset<8>)val).count();
      } else if (r - 7 < min_y) {
        uchar val = val_table.constScanLine(r)[c];
        int valid = r - min_y + 1;
        int overflow = 8 - valid;
        val = val >> overflow << overflow;
        column_payload.append(val);
        column_count += ((std::bitset<8>)val).count();
      } else {
        uchar val = val_table.constScanLine(r)[c];
        column_payload.append(val);
        column_count += ((std::bitset<8>)val).count();
      }
    }
    if (column_count > 0) {
      if (min_data_idx < 0) {
        min_data_idx = i;
      }
      max_data_idx = i;
      px_count += column_count;
    }
    payload_data.append(column_payload);
  }
  if (min_data_idx < 0) {
    return PacketData{};
  }
  min_data_idx = qMax(min_data_idx - padding_left, 0);
  max_data_idx = qMin(max_data_idx + padding_right, w - 1);
  int new_w = max_data_idx - min_data_idx + 1;
  int new_x = reverse_x ? (x + w - 1 - max_data_idx) : (x + min_data_idx);
  QRect adjusted_box(new_x, y, new_w, h);

  QByteArray payload;
  payload.append((const char*)(&new_w), 4);
  payload.append((const char*)(&slice_height), 4);
  payload.append(8, (char)0);  // x, y
  if (!is_4c) {
    payload.append(4, (char)0);  // reserved
  }
  for (int i = min_data_idx; i <= max_data_idx; i++) {
    payload.append(payload_data[i]);
  }
  return PacketData{px_count, adjusted_box, payload};
}

void PrinterBitmapFactory::write_payload(NozzleMode printer_packet_type,
                                         const QByteArray& payload) {
  proc->write_printer_packet(int(printer_packet_type), payload, !is_4c, is_4c);
}

void PrinterBitmapFactory::write_data_to_proc(const SlicedBox& box,
                                              const QByteArray& payload,
                                              int px_count,
                                              float speed,
                                              bool reverse_x,
                                              bool force_y,
                                              NozzleMode nozzle_mode) {
  if (nozzle_mode == NozzleMode::UNDEFINED) {
    nozzle_mode = NozzleMode::RIGHT;
  }
  int pixel_x = box.x();
  int pixel_y = box.y();
  int w = box.width();
  int start_x = reverse_x ? (pixel_x + w) : pixel_x;
  int end_x = reverse_x ? pixel_x : (pixel_x + w);
  QPointF real_pos =
      pixel_to_actual_position(start_x, pixel_y, nozzle_mode, reverse_x);
  proc->moveto(NamedArgs()
                   .rx(real_pos.x())
                   .ry(real_pos.y())
                   .rs(0)
                   .set_force_y(force_y)
                   .set_is_travel());
  write_payload(nozzle_mode, payload);
  proc->set_printer_packet_px_count(px_count);
  real_pos = pixel_to_actual_position(end_x, pixel_y, nozzle_mode, reverse_x);
  proc->moveto(NamedArgs()
                   .rx(real_pos.x())
                   .ry(real_pos.y())
                   .rs(1)
                   .rf(speed)
                   .set_force_y(force_y));
  proc->moveto(NamedArgs().rs(0));
}
