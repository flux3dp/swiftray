#include "uv.h"
#include "factory-utils.h"
#include "toolpath_exporter/toolpath-exporter-constants.h"
#include "toolpath_exporter/toolpath-utils.h"

UVBitmapFactory::UVBitmapFactory(FactoryKwargs& kwargs) noexcept
    : PrinterBitmapFactory(std::move(normalize(kwargs)), 0, 12, 0, 0) {
  qInfo() << "UVBitmapFactory created";
  is_4c = true;  // use 4c protocol for UV printing
}

void UVBitmapFactory::set_uv_type(UVType val) {
  uv_type = val;
}

void UVBitmapFactory::set_uv_x_step(int val) {
  x_step = val;
}

// This is setter
void UVBitmapFactory::set_uv_light_strength(int val) {
  uv_light_strength = val;
}

void UVBitmapFactory::set_uv_curing_after(bool val) {
  uv_curing_after = val;
}

void UVBitmapFactory::set_printing_repeat(int val) {
  printing_repeat = val;
}

void UVBitmapFactory::set_uv_curing_repeat(int val) {
  uv_curing_repeat = val;
}

// set_uv_light_strength in client
void UVBitmapFactory::write_uv_light_strength(int val) {
  int strength = qMax(0, qMin(100, val));
  QByteArray payload;
  payload.append(2, (char)2);  // command type for setting UV strength
  payload.append((const char*)(&strength), 2);
  proc->write_printer_packet(4, payload, false, is_4c);
}

PacketData UVBitmapFactory::create_image_packet_data(const SlicedBox& box,
                                                     int padding_left,
                                                     int padding_right,
                                                     int current_step,
                                                     bool reverse_x) {
  bool reverse_y = false;
  int x = box.x();
  int y = box.y();
  int w = box.width();
  int h = box.height();
  int padding_top = box.padding_top;

  int row_number = slice_height / interpolation;
  int min_data_idx = -1;
  int max_data_idx = -1;  // included
  int max_y = y + h;      // excluded
  int min_y = y + padding_top;

  int r = (row_number + 7) / 8;
  QByteArray empty_column;
  empty_column.append(r, (char)0);

  int column_count, i, j, c, bit_idx, cur_val;
  QVector<QByteArray> payload_data;
  int px_count = 0;
  for (i = 0; i < w; i++) {
    if (i % x_step != current_step) {
      payload_data.append(empty_column);
      continue;
    }
    c = reverse_x ? (x + w - 1 - i) : (x + i);
    column_count = 0;
    QByteArray column_payload;
    bit_idx = 7;
    cur_val = 0;
    for (j = 0; j < row_number; j++) {
      r = reverse_y ? (y + slice_height - 1 - j * interpolation)
                    : (y + j * interpolation);
      if (r >= min_y && r < max_y && bitmap.constScanLine(r)[c] < WHITE_PIXEL) {
        cur_val += (1 << bit_idx);
        column_count += 1;
      }
      bit_idx -= 1;
      if (bit_idx < 0) {
        column_payload.append(cur_val);
        bit_idx = 7;
        cur_val = 0;
      }
    }
    if (bit_idx != 7) {
      column_payload.append(cur_val);
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
  for (i = min_data_idx; i <= max_data_idx; i++) {
    payload.append(payload_data[i]);
  }
  return PacketData{px_count, adjusted_box, payload};
}

void UVBitmapFactory::write_payload(NozzleMode printer_packet_type,
                                    const QByteArray& payload) {
  int packet_type = 0;
  if (uv_type == UVType::WHITE_INK) {
    packet_type = 5;
  } else if (uv_type == UVType::VARNISH) {
    packet_type = 6;
  } else {
    // error
  }
  proc->m137_cmd_type1(1, NamedArgs().rf(0).rs(float(pixel_size)));
  proc->write_printer_packet(packet_type, payload, !is_4c, is_4c);
}

void UVBitmapFactory::write_data_to_proc(const SlicedBox& box,
                                         const QByteArray& payload,
                                         int px_count,
                                         float speed,
                                         bool reverse_x,
                                         bool force_y,
                                         NozzleMode nozzle_mode) {
  bool need_uv = !uv_curing_after && uv_light_strength > 0;
  if (need_uv) {
    write_uv_light_strength(uv_light_strength);
  }
  PrinterBitmapFactory::write_data_to_proc(box, payload, px_count, speed,
                                           reverse_x, force_y, nozzle_mode);
  if (need_uv) {
    write_uv_light_strength(0);
  }
}

void UVBitmapFactory::uv_cure_rows(QVector<RowBoxes> rows, float speed) {
  if (uv_light_strength <= 0) {
    return;
  }
  bool reverse_x = false;
  for (const RowBoxes& row : rows) {
    int start_x = -1;
    int end_x = -1;
    int y;
    for (const SlicedBox& seg : row) {
      int x = seg.x();
      y = seg.y();
      int w = seg.width();
      if (start_x < 0 || x < start_x) {
        start_x = x;
      }
      if (x + w > end_x) {
        end_x = x + w;
      }
    }
    if (start_x < 0 || end_x < 0) {
      continue;
    }
    QPointF start_pos =
        pixel_to_actual_position(start_x, y, NozzleMode::UNDEFINED);
    QPointF end_pos = pixel_to_actual_position(end_x, y, NozzleMode::UNDEFINED);
    if (reverse_x) {
      std::swap(start_pos, end_pos);
    }
    proc->moveto(NamedArgs().rx(start_pos.x()).ry(start_pos.y()).set_is_travel());
    write_uv_light_strength(uv_light_strength);
    proc->moveto(NamedArgs().rx(end_pos.x()).ry(end_pos.y()).rf(speed));
    proc->sync_grbl_motion(0);
    write_uv_light_strength(0);
    reverse_x = !reverse_x;
  }
}

void UVBitmapFactory::generate_task_code(GenerateTaskKwargs kwargs) {
  if (kwargs.repeat == 0 || workspaces->isEmpty()) {
    return;
  }

  double padding_dist_left = qMax(kwargs.padding_dist_left, kwargs.padding_dist);
  double padding_dist_right = qMax(kwargs.padding_dist_right, kwargs.padding_dist);
  int padding_left = get_padding_pixels(padding_dist_left);
  int padding_right = get_padding_pixels(padding_dist_right);
  generate_image_for_task(kwargs.black_ratio);

  QVector<QRect> contour_boxes =
      get_bounding_boxes(&bitmap, bitmap_dirty_area, padding_left,
                         padding_right, slice_height, split_bbox);

  QVector<BlockBoxes> blocks = {};
  for (auto box : contour_boxes) {
    blocks.append(slice_image(box, kwargs.reverse_y, kwargs.multipass));
  }

  // Generate fcode
  QVector<RowBoxes> uv_curing_rows;
  NamedArgs args_s0 = NamedArgs().rs(0);
  bool reverse_x = false;
  bool no_cache = true;
  for (int r = 0; r < kwargs.repeat; r++) {
    for (int rr = 0; rr < printing_repeat; rr++) {
      uv_curing_rows.clear();
      for (BlockBoxes& block : blocks) {
        reverse_x = false;
        for (RowBoxes& row : block) {
          RowBoxes cur_row_task_bboxes;
          int row_size = row.size();
          for (int i = 0; i < row_size; i++) {
            int real_i = reverse_x ? (row_size - 1 - i) : i;
            SlicedBox bbox = row[real_i];
            for (int step = 0; step < x_step; step++) {
              if (no_cache) {
                bbox.data.append(create_image_packet_data(
                    bbox, padding_left, padding_right, step, reverse_x));
              }
              PacketData data = bbox.data[step];
              if (data.px_count == 0) {
                continue;
              }
              SlicedBox task_bbox(data.box, bbox.padding_top);
              write_data_to_proc(task_bbox,
                                 data.payload,  // always use normal payload
                                 data.px_count, kwargs.speed, reverse_x);
              cur_row_task_bboxes.append(task_bbox);
            }
          }
          proc->moveto(args_s0);
          if (!one_way) {
            reverse_x = !reverse_x;
          }
          if (!cur_row_task_bboxes.isEmpty() && uv_curing_after) {
            uv_curing_rows.append(cur_row_task_bboxes);
          }
        }
      }
    }
    if (uv_curing_after) {
      if (printing_repeat < 1) {
        uv_curing_rows.clear();
        for (BlockBoxes& block : blocks) {
          for (RowBoxes& row : block) {
            uv_curing_rows.append(row);
          }
        }
      }
      for (int cure_repeat = 0; cure_repeat < uv_curing_repeat; cure_repeat++) {
        uv_cure_rows(uv_curing_rows, kwargs.speed);
      }
    }
  }
}
