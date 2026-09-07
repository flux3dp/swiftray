#include "printer-4c.h"
#include "factory-utils.h"
#include "toolpath_exporter/toolpath-utils.h"

PrinterBitmapFactory4C::PrinterBitmapFactory4C(FactoryKwargs& kwargs) noexcept
    : PrinterBitmapFactory(std::move(normalize(kwargs)),
                           DEFAULT_SLICE_WIDTH_4C,
                           DEFAULT_SLICE_HEIGHT_4C,
                           0,
                           0) {
  qInfo() << "PrinterBitmapFactory4C created";
  is_4c = true;
  colors = {PrintingColor::CYAN, PrintingColor::MAGENTA, PrintingColor::YELLOW,
            PrintingColor::BLACK};
  prespray_width = 32;
  prespray_safe_x = 0;
  nozzle_mode_ = NozzleMode::BOTH;

  color_curves_map = COLOR_CURVES_MAP_4C[halftone - 1];
}

void PrinterBitmapFactory4C::set_reversed(bool val) {
  if (val != is_reversed) {
    reverse_offset();
    is_reversed = val;
  }
}

void PrinterBitmapFactory4C::reverse_offset() {
  int max_x = 0;
  int max_y = 0;
  auto color_offsets_values = color_offsets.values();
  for (const auto& offset : color_offsets_values) {
    max_x = qMax(max_x, offset.x());
    max_y = qMax(max_y, offset.y());
  }
  for (auto& offset : color_offsets) {
    offset.setX(max_x - offset.x());
    offset.setY(max_y - offset.y());
  }
}

void PrinterBitmapFactory4C::set_am_angle_map(const QString& raw_val) {
  // example:{"c":75,"k":15,"m":45,"y":90}
  QJsonDocument doc = QJsonDocument::fromJson(raw_val.toUtf8());
  if (doc.isNull() || !doc.isObject()) {
    return;
  }
  QJsonObject obj = doc.object();
  QMap<PrintingColor, double> map;
  for (PrintingColor color : colors) {
    QString color_str((const char*)&color);
    if (!obj.contains((color_str)) || !obj.value(color_str).isDouble()) {
      return;
    }
    map[color] = obj[color_str].toDouble();
  }
  am_angle_map = map;
}

void PrinterBitmapFactory4C::set_color_curves_map(const QString& raw_val) {
  // example:{"c":[0,88,170,229,255],"k":[0,60,109,163,207],"m":[0,90,149,204,241],"y":[0,96,147,186,249]}
  QJsonDocument doc = QJsonDocument::fromJson(raw_val.toUtf8());
  if (doc.isNull() || !doc.isObject()) {
    return;
  }
  QJsonObject obj = doc.object();
  QMap<PrintingColor, QVector<int>> map;
  for (PrintingColor color : colors) {
    QString color_str((const char*)&color);
    if (!obj.contains((color_str)) || !obj.value(color_str).isArray()) {
      return;
    }
    QJsonArray data = obj[color_str].toArray();
    QVector<int> curve;
    for (const QJsonValue& v : data) {
      curve.append(v.toInt());
    }
    map[color] = curve;
  }
  color_curves_map = map;
}

void PrinterBitmapFactory4C::set_macros(
    std::shared_ptr<BaseMacros> macros_ptr) {
  macros = macros_ptr;
}

void PrinterBitmapFactory4C::set_refresh_x_mm(double val) {
  if (isnan(val) || val < 0) {
    return;
  }
  refresh_x_mm = val;
}

void PrinterBitmapFactory4C::set_refresh_interval(int val) {
  refresh_interval = val;
}

void PrinterBitmapFactory4C::set_burst_refresh(bool val) {
  burst_refresh = val;
}

QPointF PrinterBitmapFactory4C::pixel_to_actual_position(int x,
                                                         int y,
                                                         NozzleMode nozzle_mode,
                                                         bool apply_backlash) {
  QPointF pos = PrinterBitmapFactory::pixel_to_actual_position(
      x, y, nozzle_mode, apply_backlash);
  if (nozzle_mode == NozzleMode::RIGHT) {
    // default offset for right nozzle
    QPointF right_offset(0.55035, 0);
    return is_reversed ? (pos - right_offset) : (pos + right_offset);
  }
  return pos;
}

void PrinterBitmapFactory4C::add_image_by_color(QImage& img,
                                                QRectF& bbox,
                                                QColor color) {
  int c, m, y, k;
  color.getCmyk(&c, &m, &y, &k);
  PrintingColor printing_color;
  if (c) {
    printing_color = PrintingColor::CYAN;
  } else if (m) {
    printing_color = PrintingColor::MAGENTA;
  } else if (y) {
    printing_color = PrintingColor::YELLOW;
  } else {
    printing_color = PrintingColor::BLACK;
  }
  int color_index = colors.indexOf(printing_color);
  if (color_index == -1) {
    return;
  }
  QRectF offseted_bbox(bbox.x() - color_offsets[printing_color].x(),
                       bbox.y() - color_offsets[printing_color].y(),
                       bbox.width(), bbox.height());
  add_image(img, offseted_bbox, color_index);
}

void PrinterBitmapFactory4C::align_workspace_dimensions() {
  for (int i = 0; i < colors.size(); i++) {
    auto workspace = get_workspace(i);
    bitmap_dirty_area = bitmap_dirty_area.united(workspace->get_dirty_area());
  }
}

void PrinterBitmapFactory4C::generate_prespray_task_code(
    float speed,
    bool reverse_x,
    NozzleMode nozzle_mode) {
  if (!prespray_bbox.isValid()) {
    return;
  }
  generate_4_color_block(prespray_bbox, speed, reverse_x, nozzle_mode);
}

void PrinterBitmapFactory4C::generate_4_color_block(QRect box,
                                                    float speed,
                                                    bool reverse_x,
                                                    NozzleMode nozzle_mode,
                                                    int multipass) {
  int x = box.x();
  int y = box.y();
  int width = box.width();
  int height = box.height();
  int len_colors = colors.size();
  int pixels_per_color = width / len_colors;
  bitmap = QImage(width, height, QImage::Format_Grayscale8);
  bitmap.fill(Qt::white);
  for (int i = 0; i < len_colors; i++) {
    PrintingColor color = colors[i];
    QPoint offset = color_offsets[color];
    int start_x = i * pixels_per_color - offset.x();
    int end_x = start_x + pixels_per_color;
    uchar color_value = 1 << (len_colors - 1 - i);
    for (int row = 0; row < height; row++) {
      uchar* data_ptr = bitmap.scanLine(row);
      for (int col = qMax(start_x, 0); col < end_x; col++) {
        data_ptr[col] -= color_value;
      }
    }
  }
  QVector<SlicedBox> boxes;
  int cols = width / slice_width + 1;
  for (int i = 0; i < cols; i++) {
    int w = (i < cols - 1) ? slice_width : width % slice_width;
    int h = qMin(slice_height, height);
    if (w <= 0 || h <= 0) {
      continue;
    }
    boxes.append(SlicedBox(i * slice_width, 0, w, h, 0));
  }
  if (reverse_x) {
    std::reverse(boxes.begin(), boxes.end());
  }
  bool did_start = false;
  int len_boxes = boxes.size();
  for (int i = 0; i < len_boxes; i++) {
    SlicedBox box = boxes[i];
    PacketData4C data = create_image_packet_data_4c(box, reverse_x);
    if (data.payload.isEmpty()) {
      continue;
    }
    SlicedBox real_box(box.x() + x, box.y() + y, box.width(), box.height(),
                       box.padding_top);
    bool is_end = (i == len_boxes - 1);
    write_data_to_proc_4c(real_box, data.payload, speed, reverse_x, false,
                          !did_start, is_end, false, nozzle_mode);
    did_start = true;
  }
  proc->wait_printer_mode_sync();
}

void PrinterBitmapFactory4C::refresh_ink(int repeat,
                                         double block_width_mm,
                                         double y) {
  int block_width = block_width_mm * pixel_per_mm;
  int color_size = colors.size();
  QVector<QVector<bool>> nozzle_data(color_size);
  for (int i = 0; i < color_size; i++) {
    nozzle_data[i].fill(true, DEFAULT_SLICE_HEIGHT_4C);
  }
  bool need_refresh_left =
      nozzle_mode_ == NozzleMode::BOTH || nozzle_mode_ == NozzleMode::LEFT;
  bool need_refresh_right =
      nozzle_mode_ == NozzleMode::BOTH || nozzle_mode_ == NozzleMode::RIGHT;
  if (need_refresh_left || need_refresh_right) {
    bool use_macros_refresh = std::isnan(refresh_x_mm);
    float x = NAN;
    if (!use_macros_refresh) {
      x = refresh_x_mm - offset.x();
      if (std::isnan(y)) {
        proc->moveto(NamedArgs().rx(x).rs(0).set_is_travel());
      } else {
        proc->moveto(NamedArgs().rx(x).ry(y).rs(0).set_is_travel());
      }
    } else if (hasattr(macros, MacroFunc::move_to_refresh_position)) {
      QPointF pos = macros->move_to_refresh_position();
      x = pos.x();
      proc->moveto(
          NamedArgs().rx(x).ry(pos.y()).rs(0).set_force_y().set_is_travel());
    }
    if (!isnan(x)) {
      float end_x = x + (block_width + 1) * pixel_size;
      bool is_going_back = false;
      auto generate_payload = [](const QVector<QVector<bool>>& data,
                                 int w) -> QByteArray {
        QByteArray refresh_payload;
        int color_size = data.size();   // should be 4
        int data_len = data[0].size();  // should be DEFAULT_SLICE_HEIGHT_4C
        uchar val = 0;
        for (int y = 0; y < data_len; y++) {
          for (int c = 0; c < color_size; c++) {
            val = (val << 1) | (data[c][y] ? 1 : 0);
          }
          if (y % 2) {
            refresh_payload.append(val);
            val = 0;
          }
        }
        QByteArray payload;
        payload.append((const char*)(&w), 4);
        payload.append((const char*)(&data_len), 4);
        payload.append(8, (char)0);
        for (int i = 0; i < w; i++) {
          payload.append(refresh_payload);
        }
        return payload;
      };
      if (burst_refresh) {
        proc->sync_grbl_motion(0);
        proc->burst_refresh(burst_refresh_counts);
        proc->sync_grbl_motion(0);
      } else {
        if (nozzle_mode_ == NozzleMode::BOTH ||
            nozzle_mode_ == NozzleMode::LEFT) {
          QByteArray payload = generate_payload(nozzle_data, block_width);
          for (int i = 0; i < repeat; i++) {
            write_payload(NozzleMode::LEFT, payload);
            if (!is_going_back) {
              proc->moveto(NamedArgs().rx(end_x).rs(1).rf(1800));
            } else {
              proc->moveto(NamedArgs().rx(x).rs(1).rf(1800));
            }
            proc->wait_printer_mode_sync();
            is_going_back = !is_going_back;
          }
        }
        proc->moveto(NamedArgs().rs(0));
        if (nozzle_mode_ == NozzleMode::BOTH ||
            nozzle_mode_ == NozzleMode::RIGHT) {
          QByteArray payload = generate_payload(nozzle_data, block_width);
          for (int i = 0; i < repeat; i++) {
            write_payload(NozzleMode::RIGHT, payload);
            if (!is_going_back) {
              proc->moveto(NamedArgs().rx(end_x).rs(1).rf(1800));
            } else {
              proc->moveto(NamedArgs().rx(x).rs(1).rf(1800));
            }
            proc->wait_printer_mode_sync();
            is_going_back = !is_going_back;
          }
        }
      }
      proc->moveto(NamedArgs().rx(x).rs(0).set_is_travel());
      proc->wait_printer_mode_sync();
      if (use_macros_refresh &&
          hasattr(macros, MacroFunc::post_refresh_motion)) {
        macros->post_refresh_motion();
      }
    }
  }
}

PacketData4C PrinterBitmapFactory4C::create_image_packet_data_4c(
    const SlicedBox& box,
    bool reverse_x,
    bool skip_empty) {
  int x = box.x();
  int y = box.y();
  int w = box.width();
  int h = box.height();
  int padding_top = box.padding_top;

  int bit_idx, cur_val;
  QVector<QByteArray> payload_data;
  int i, c, r, dist, val, count;
  bool has_data = false;
  const int max_y = qMin(y + h, bitmap.height());  // excluded
  const int min_y = qMax(y + padding_top, 0);     // included
  int bit_count = 4;
  int init_idx = 8 - bit_count;

  for (i = 0; i < w; i++) {
    c = reverse_x ? (x + w - 1 - i) : (x + i);
    QByteArray column_payload;
    bit_idx = init_idx;
    cur_val = 0;
    for (r = y; r < y + slice_height; r++) {
      if (r >= min_y && r < max_y) {
        const uchar* data_ptr = bitmap.constScanLine(r);
        uchar pixel_value = WHITE_PIXEL - data_ptr[c];
        if (pixel_value > 0) {
          cur_val += pixel_value << bit_idx;
          has_data = true;
        }
      }
      bit_idx -= bit_count;
      // Note: this might be wrong if 8 % bit_count != 0, but it's not the case
      // here
      if (bit_idx < 0) {
        column_payload.append(cur_val);
        bit_idx = init_idx;
        cur_val = 0;
      }
    }
    if (bit_idx != init_idx) {
      column_payload.append(cur_val);
    }
    payload_data.append(column_payload);
  }

  if (!has_data && skip_empty) {
    return PacketData4C{};
  }
  QByteArray payload;
  payload.append((const char*)(&w), 4);
  payload.append((const char*)(&slice_height), 4);
  payload.append(8, (char)0);  // x, y
  for (i = 0; i < w; i++) {
    payload.append(payload_data[i]);
  }
  return PacketData4C{payload};
}

void PrinterBitmapFactory4C::generate_task_code(GenerateTaskKwargs kwargs) {
  if (kwargs.repeat == 0) {
    return;
  }
  align_workspace_dimensions();
  if (!bitmap_dirty_area.isValid()) {
    return;
  }
  const QRect work_area_rect(QPoint(0, 0), work_area);
  const QRect dirty_rect = getImageBBox(bitmap_dirty_area, work_area_rect);
  if (dirty_rect.isEmpty()) {
    bitmap_dirty_area = QRectF();
    return;
  }
  bitmap_dirty_area = QRectF(dirty_rect);
  double padding_dist_l = qMax(kwargs.padding_dist_left, kwargs.padding_dist);
  double padding_dist_r = qMax(kwargs.padding_dist_right, kwargs.padding_dist);
  int padding_pixels_l = get_padding_pixels(padding_dist_l);
  int padding_pixels_r = get_padding_pixels(padding_dist_r);

  // color_curve and halftone
  const RectBorders borders = getRectBorders(dirty_rect);

  bool do_am = halftone > 1;
  double smoother = halftone_params.smoother;
  double multiplier;
  double am_cos, am_sin, am_dot_r, am_dot_d;
  if (do_am) {
    am_dot_r = qMax(pixel_per_mm / (2.0 * halftone_params.density), 1.0);
    am_dot_d = am_dot_r * 2;
  }

  int color_size = colors.size();
  for (int i = 0; i < color_size; i++) {
    auto color = colors[i];
    auto workspace = get_workspace(i);
    if (workspace->get_dirty_area().isEmpty()) {
      continue;
    }
    color_curve = color_curves_map.value(color, {});
    bool do_color_curve = !color_curve.isEmpty();

    multiplier = halftone_params.color_multipliers[color] * kwargs.black_ratio;
    if (do_am) {
      double rad = am_angle_map.value(color, 75) * M_PI / 180;
      am_cos = cos(rad);
      am_sin = sin(rad);
    }

    QImage* src_bitmap = workspace->get_bitmap();
    for (int y = borders.top; y < borders.bottom_exclusive; y++) {
      uchar* data_ptr = src_bitmap->scanLine(y);
      for (int x = borders.left; x < borders.right_exclusive; x++) {
        int inv_val = WHITE_PIXEL - data_ptr[x];
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
          inv_val = am_halftone(inv_val, x, y, am_cos, am_sin, am_dot_r,
                                am_dot_d, smoother, multiplier);
        } else {
          inv_val = fm_halftone(inv_val, smoother, multiplier);
        }
        data_ptr[x] = (uchar)(WHITE_PIXEL - inv_val);
      }
    }
    if (!do_am) {
      src_bitmap->convertTo(QImage::Format_Mono, Qt::DiffuseDither);
      src_bitmap->convertTo(QImage::Format_Grayscale8);
    }
  }

  // get_image_data
  bitmap = QImage(work_area, QImage::Format_Grayscale8);
  bitmap.fill(Qt::white);
  for (int i = 0; i < color_size; i++) {
    auto workspace = get_workspace(i);
    if (workspace->get_dirty_area().isEmpty()) {
      continue;
    }
    uchar bit = 1 << (color_size - i - 1);
    QImage* src_bitmap = workspace->get_bitmap();
    for (int y = borders.top; y < borders.bottom_exclusive; y++) {
      uchar* data_ptr = bitmap.scanLine(y);
      uchar* src_data_ptr = src_bitmap->scanLine(y);
      for (int x = borders.left; x < borders.right_exclusive; x++) {
        if (src_data_ptr[x] != WHITE_PIXEL)
          data_ptr[x] -= bit;
      }
    }
  }
  // int total_block = 0;
  QVector<QRect> contour_boxes = get_bounding_boxes(
      &bitmap, bitmap_dirty_area,
      qMax(padding_pixels_l, DEFAULT_SLICE_WIDTH_4C),
      qMax(padding_pixels_r, DEFAULT_SLICE_WIDTH_4C), slice_height, split_bbox);
  QVector<BlockBoxes> blocks = {};
  for (auto box : contour_boxes) {
    blocks.append(slice_image(box, kwargs.reverse_y, kwargs.multipass));
    // total_block += blocks.constLast().size();
  }
  if (this->cancelled)
    return;
  onProgressChanged(0.05, true);

  bool should_refresh = true;
  float last_refresh_time = proc->get_time_cost();
  int row_counts = 0;

  double start_padding = padding_dist_l;
  double end_padding = padding_dist_r;
  bool no_cache = true;
  NamedArgs args_s0 = NamedArgs().rs(0);
  for (int r = 0; r < kwargs.repeat; r++) {
    for (BlockBoxes& block : blocks) {
      bool reverse_x = false;
      start_padding = padding_dist_l;
      end_padding = padding_dist_r;
      for (RowBoxes& row : block) {
        int row_size = row.size();
        bool just_refreshed = false;
        double final_x = NAN;
        bool is_printing = false;     // Determine continuous printing is_start
        bool is_row_printed = false;  // Determine row refreshing

        for (int i = 0; i < row_size; i++) {
          int real_i = reverse_x ? (row_size - 1 - i) : i;
          SlicedBox box = row[real_i];
          if (no_cache) {
            box.data_4c =
                create_image_packet_data_4c(box, reverse_x);
          }
          PacketData4C data = box.data_4c;
          if (data.payload.isEmpty()) {
            if (is_printing) {
              is_printing = false;
              proc->moveto(args_s0);
            }
            continue;
          }
          NozzleMode nozzle_mode = nozzle_mode_;
          if (nozzle_mode == NozzleMode::BOTH) {
            nozzle_mode =
                (row_counts % 2 == 0) ? NozzleMode::LEFT : NozzleMode::RIGHT;
          }
          if (should_refresh) {
            double refresh_y =
                pixel_to_actual_position(box.x(), box.y(), nozzle_mode, false)
                    .y();
            refresh_ink(3, 5, refresh_y);
            should_refresh = false;
            last_refresh_time = proc->get_time_cost();
            just_refreshed = true;
          }
          bool is_start = !is_printing || just_refreshed;
          is_printing = true;
          bool is_end = (i == row_size - 1);
          final_x = write_data_to_proc_4c(
              box, data.payload, kwargs.speed, reverse_x, false, is_start,
              is_end, just_refreshed && isnan(refresh_x_mm), nozzle_mode,
              is_row_printed ? 0 : start_padding);
          just_refreshed = false;
          is_row_printed = true;
          if (macros && refresh_interval > 0 &&
              proc->get_time_cost() - last_refresh_time > refresh_interval) {
            should_refresh = true;
            proc->wait_printer_mode_sync();
            proc->moveto(args_s0);
          }
        }
        proc->moveto(args_s0);
        if (!isnan(final_x) && end_padding > 0) {
          float padded_x = get_padded_x(final_x, false, reverse_x, end_padding);
          proc->moveto(NamedArgs().rx(padded_x).rf(kwargs.speed).set_is_travel());
        }
        if (is_row_printed) {
          row_counts += 1;
        }
        if (!one_way) {
          reverse_x = !reverse_x;
          std::swap(start_padding, end_padding);
        }
      }
    }
    no_cache = false;
  }
}

float PrinterBitmapFactory4C::get_padded_x(double x,
                                           bool is_start,
                                           bool reverse_x,
                                           double padding) {
  if (padding <= 0)
    return x;
  if (is_start != reverse_x)
    return qMax(x - padding, qMin(clip_rect.x() / pixel_per_mm, x));
  else
    return qMin(x + padding, qMax(clip_rect.right() / pixel_per_mm, x));
}

double PrinterBitmapFactory4C::write_data_to_proc_4c(
    const SlicedBox& box,
    const QByteArray& payload,
    float speed,
    bool reverse_x,
    bool force_y,
    bool is_start,
    bool is_end,
    bool should_adjust_height,
    NozzleMode nozzle_mode,
    double padding) {
  if (nozzle_mode == NozzleMode::UNDEFINED) {
    nozzle_mode = NozzleMode::LEFT;
  }
  int pixel_x = box.x();
  int pixel_y = box.y();
  int w = box.width();
  int start_x = reverse_x ? (pixel_x + w) : pixel_x;
  int end_x = reverse_x ? pixel_x : (pixel_x + w);
  end_x = reverse_x ? (end_x - 1) : (end_x + 1);
  QPointF real_pos;
  if (is_start) {
    real_pos =
        pixel_to_actual_position(start_x, pixel_y, nozzle_mode, reverse_x);
    if (padding > 0) {
      proc->moveto(NamedArgs()
                       .rx(get_padded_x(real_pos.x(), true, reverse_x, padding))
                       .ry(real_pos.y())
                       .rs(0)
                       .set_force_y(force_y)
                       .set_is_travel());
      proc->moveto(NamedArgs().rx(real_pos.x()).rf(speed).set_is_travel());
    } else {
      proc->moveto(NamedArgs()
                       .rx(real_pos.x())
                       .ry(real_pos.y())
                       .rs(0)
                       .set_force_y(force_y)
                       .set_is_travel());
    }
  }
  if (should_adjust_height) {
    proc->sync_motion_type2(185, 0);
    proc->sync_motion_type2(179, 2);
  }
  write_payload(nozzle_mode, payload);
  real_pos = pixel_to_actual_position(end_x, pixel_y, nozzle_mode, reverse_x);
  proc->moveto(NamedArgs()
                   .rx(real_pos.x())
                   .ry(real_pos.y())
                   .rs(1)
                   .rf(speed)
                   .set_force_y(force_y));
  if (is_end) {
    proc->wait_printer_mode_sync();
    proc->moveto(NamedArgs().rs(0));
  }

  return real_pos.x();
}
