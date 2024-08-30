#include <constants.h>
#include <toolpath_exporter/toolpath-exporter-fcode.h>
#include <windows/image-sharpen-dialog.h>
#include <QBuffer>
#include <QCoreApplication>
#include <QDebug>
#include <QElapsedTimer>
#include <QJsonValue>
#include <QProgressDialog>
#include <QVector2D>
#include <QtMath>
#include <bitset>
#include <boost/range/irange.hpp>
#include <cmath>
#include <iomanip>
#include <iostream>

float positiveMod(float n, float m) {
  float val = std::fmod(n, m);
  if (val < 0) {
    val = std::fmod(val + m, m);
  }
  return val;
}

float getAngle(QVector2D v1, QVector2D v2) {
  int direction_sign = v1.x() * v2.y() - v1.y() * v2.x() < 0 ? -1 : 1;
  float cos_val = QVector2D::dotProduct(v1, v2) / (v1.length() * v2.length());
  return direction_sign * acos(qMax(qMin(cos_val, float(1.0)), float(-1.0)));
}

QPointF getBladeCompensation(QPointF position,
                             QVector2D vector,
                             float radius = 0.6) {
  float r = radius / vector.length();
  return (QVector2D(position) + vector * r).toPointF();
}

QString convertUnicode(const QString s) {
  QString result;
  for (int i = 0; i < s.size(); i++) {
    const char16_t unicode = s[i].unicode();
    if (unicode > 127) {
      result += QString("\\u%1").arg(unicode, 4, 16, QChar('0'));
    } else {
      result += s[i];
    }
  }
  return result;
}

bool isIntersectWithCenter(int position1, int position2) {
  // 0 1 2
  // 3 4 5
  // 6 7 8
  if (position1 == 4 || position2 == 4) {
    return true;
  } else if (position1 == position2) {
    return false;
  } else if (position1 == 1 || position1 == 7) {
    return abs(position1 - position2) != 1;
  } else if (position1 == 3 || position1 == 5) {
    return abs(position1 - position2) != 3;
  } else if (position1 == 0) {
    return position2 == 5 || position2 == 7 || position2 == 8;
  } else if (position1 == 2) {
    return position2 == 3 || position2 == 6 || position2 == 7;
  } else if (position1 == 6) {
    return position2 == 1 || position2 == 2 || position2 == 5;
  } else {
    // position1 == 8
    return position2 == 0 || position2 == 1 || position2 == 3;
  }
}

bool ToolpathExporterFcode::convertStack(const QList<LayerPtr>& layers,
                                         QProgressDialog* dialog) {
  // Step 1. Initialize
  QElapsedTimer t;
  t.start();
  Q_ASSERT_X(!layers.empty(), "ToolpathExporterFcode", "Must input at least one layer");

  bool canceled = false;
  if (dialog != nullptr) {
    connect(dialog, &QProgressDialog::canceled, [&]() { canceled = true; });
  }

  // Step 2. Handle pre-task
  if (is_v2_) {
    gen_->start_task_script_block("xMIN", "0003");
    gen_->miscellaneous_cmd(1);
    if (is_rotary_task_) {
      moveZ(-1);
    }
    gen_->grbl_system_cmd(0);
  } else {
    gen_->home();
  }
  gen_->set_toolhead_pwm(0, true);
  if (is_v2_) {
    travel(0, 0);
  }

  // Supporting for spinning axis
  if (is_rotary_task_) {
    if (is_v2_) {
      is_a_mode_ = true;
    } else {
      travel(0, config_.spinning_axis_coord + 1);
      travel(0, config_.spinning_axis_coord - 1);
      travel(0, config_.spinning_axis_coord);
      // Set rotary mode in beambox-firmware
      pause(false);
    }
  }

  // Enable GCode Boost in beambox-firmware
  pause(true);

  if (is_v2_) {
    gen_->miscellaneous_cmd(0);
  }

  gen_->set_time_est_acc(config_.padding_acc);

  // precut
  if (config_.enable_precut) {
    current_vector_ = QVector2D(1, 0);
    QPointF precut_dest_xy = (QVector2D(config_.precut_at) + current_vector_).toPointF();
    travel(config_.precut_at);
    gen_->set_toolhead_pwm(100);
    QPointF new_dest = getBladeCompensation(precut_dest_xy, current_vector_,
                                            config_.blade_radius);
    moveto(800, new_dest.x(), new_dest.y());
    current_xy_ = precut_dest_xy;
    gen_->set_toolhead_pwm(0, true);
  }

  // End of pre-task script
  if (is_v2_) {
    gen_->end_task_script_block();
  }
  if (canceled) {
    return false;
  }

  // Step 3. Init bitmap canvas
  laser_bitmap_ =
      QPixmap(QSize(std::ceil(work_area_mm_.width() * config_.dpmm_x),
                    std::ceil(work_area_mm_.height() * config_.dpmm_y)));
  printing_bitmap_ =
      QPixmap(QSize(std::ceil(work_area_mm_.width() * config_.dpmm_printing),
                    std::ceil(work_area_mm_.height() * config_.dpmm_printing)));
  // Limit preview width to 500
  preview_bitmap_ =
      QPixmap(QSize(std::round(work_area_mm_.width() * config_.dpmm_preview),
                    std::ceil(work_area_mm_.height() * config_.dpmm_preview)));
  qInfo() << "[Canvas size]";
  qInfo() << "laser_bitmap_" << laser_bitmap_.size();
  qInfo() << "printing_bitmap_" << printing_bitmap_.size();
  qInfo() << "preview_bitmap_" << preview_bitmap_.size();

  // Step 4. Handle each layer
  int processed_layer_cnt = 0;
  int visible_layer_cnt = 1;
  int last_module;
  QString last_color;
  QString last_sub_type;
  QJsonArray post_config;
  for (auto layer_rit = layers.crbegin(); layer_rit != layers.crend();
       layer_rit++) {
    qInfo() << "[Export] Output layer: " << (*layer_rit)->name();
    if ((*layer_rit)->isVisible() && (*layer_rit)->repeat() > 0) {
      current_layer_ = *layer_rit;
      updateLayerParam();

      float layer_height = current_layer_->targetHeight();
      float layer_z_step = current_layer_->stepHeight();
      if (is_v2_) {
        gen_->write_string("TASK", 4);
        // Write transition script
        gen_->start_task_script_block("TRAN", NULL);
        if (with_module_) {
          if (is_rotary_task_) {
            moveZ(1);
          }
          QPointF tran_pos;
          if (hardware_ == ToolpathExporterFcode::HardwareType::Ador) {
            tran_pos = QPointF(215, 150);
          } else {
            tran_pos = QPointF(work_area_mm_.width() / 2, work_area_mm_.height() / 2);
          }
          if (is_rotary_task_) {
            travel(std::nanf(""), 0, true);
            travel(tran_pos.x(), std::nanf(""), true);
            travel(std::nanf(""), tran_pos.y(), true);
            gen_->sync_motion_type2(179, 128, 3.0);
          } else {
            travel(tran_pos, true);
          }
          gen_->sync_grbl_motion(0);
          gen_->flux_custom_cmd(168);
          gen_->flux_custom_cmd(174);
          gen_->user_selection_cmd(0);
          gen_->grbl_system_cmd(0);
          gen_->sync_grbl_motion(0);
          gen_->miscellaneous_cmd(0);
        }
        gen_->end_task_script_block();
        // Write main script
        gen_->start_task_script_block("MAIN", NULL);
        if (is_rotary_task_) {
          rotary_wait_move_ = true;
          rotary_y_offset_ = config_.spinning_axis_coord - module_offset_.y();
          module_offset_.setY(0);
        } else {
          gen_->sync_motion_type2(179, 128, 2.0);
        }
        gen_->sync_grbl_motion(0);
        gen_->miscellaneous_cmd(1);
      }
      if (config_.enable_diode) {
        gen_->set_toolhead_laser_module(current_layer_->isUseDiode());
      }
      if (has_focus_adjust_) {
        if (is_v2_) {
          gen_->sync_motion_type2(184, 128, focus_adjust_);
        }
      }
      if (config_.enable_autofocus && !did_home_z_ && layer_height > 0) {
        moveZ(-1);
        did_home_z_ = true;
      }
      gen_->set_toolhead_pwm(-current_layer_->power() / 100);

      if (is_printing_layer_) {
        if (!with_print_task_) {
          with_print_task_ = true;
        }
        convertPrintingLayer();
        gen_->set_toolhead_pwm(0);
        moveto(std::nanf(""), std::nanf(""), std::nanf(""), std::nanf(""), 0);
      } else {
        for (int i = 0; i < current_layer_->repeat(); i++) {
          if (has_focus_adjust_ && focus_step_ > 0 && i > 0) {
            if (is_v2_) {
              float real_z = focus_adjust_ + focus_step_ * i;
              gen_->sync_motion_type2(184, 128, real_z);
            }
          } else if (config_.enable_autofocus && layer_height > 0) {
            double target_z = 17.0 - layer_height - config_.z_offset + i * layer_z_step;
            target_z = round(qMax(qMin(target_z, 17.0), 0.0) * 100) / 100;
            moveZ(target_z);
          }
          convertLaserLayer();
          gen_->set_toolhead_pwm(0);
        }
        moveto(std::nanf(""), std::nanf(""), std::nanf(""), std::nanf(""), 0);
      }
      if (is_v2_) {
        if (has_focus_adjust_) {
          gen_->sync_motion_type2(184, 128, 0);
        }
        gen_->end_task_script_block();
        // Write task info
        QString submodule_type = "None";
        if (is_printing_layer_) {
          submodule_type = current_layer_->uv() ? "UV" : "Solvent";
        }
        QJsonObject task_info{{"idx", visible_layer_cnt},
                              {"name", convertUnicode(current_layer_->name())},
                              {"head_type", layer_module_},
                              {"display_color", layer_color_},
                              {"submodule", QJsonObject{
                                                {"type", submodule_type},
                                                {"color", submodule_color_},
                                            }}};
        gen_->write_task_info(task_info);
        writePreviewImage();
        // Update post script
        bool need_transition = false;
        if (!last_color.isEmpty()) {
          if (last_module != layer_module_) {
            // module changed
            need_transition = true;
          } else if (is_printing_layer_ && (last_color != layer_color_ ||
                                            last_sub_type != submodule_type)) {
            // printing color or submodule changed
            need_transition = true;
          }
        }
        post_config.append(QJsonObject{
            {"idx", visible_layer_cnt},
            {"tran", need_transition ? 1 : 0},
            {"uv", submodule_type == "UV" ? 1 : 0},
        });
        last_module = layer_module_;
        last_color = layer_color_;
        last_sub_type = submodule_type;
      }
      visible_layer_cnt++;
    }
    if (canceled) {
      break;
    }
    processed_layer_cnt++;
    if (dialog != nullptr) {
      dialog->setValue(100 * processed_layer_cnt / layers.count());
      QCoreApplication::processEvents();
    }
  }
  if (canceled) {
    return false;
  }

  // Step 5. Handle printing test, prespray task if needed
  if (is_v2_ && with_print_task_ && !config_.prespray.isEmpty()) {
    // Mock layer param for printing dpmm and module offset
    layer_module_ = 5;
    is_printing_layer_ = true;
    updateOffset();
    updateClip();
    auto [prespray_bbox, test_bbox] = getPresprayBbox();
    float x = config_.prespray.x();
    float y = config_.prespray.y();
    QByteArray prespray_payload = generateNozzleSettingPayload(9, true);
    gen_->start_task_script_block("xMIN", "0001");
    gen_->grbl_system_cmd(0);
    setTravelSpeed(config_.prespray_travel_speed);
    if (is_rotary_task_) {
      travel(x, std::nanf(""), true, 0);
      travel(std::nanf(""), y, true, 0);
      moveZ(35);
    } else {
      travel(x, y, true, 0);
    }
    gen_->enter_printer_mode();

    // prespray before test
    gen_->start_printer_packet(17);
    gen_->write_printer_packet(prespray_payload);
    gen_->end_printer_packet();
    writeSimpleFilledTaskCode(prespray_bbox);
    gen_->wait_printer_mode_sync();

    // printing test
    QByteArray payload = generateNozzleSettingPayload(1, true);
    gen_->start_printer_packet(17);
    gen_->write_printer_packet(payload);
    gen_->end_printer_packet();
    writeCatridgeTaskCode(test_bbox);
    setTravelSpeed(config_.travel_speed);
    gen_->wait_printer_mode_sync();
    gen_->exit_printer_mode();
    moveto(config_.travel_speed, std::nanf(""), std::nanf(""), std::nanf(""), 0);
    if (is_rotary_task_) {
      moveZ(1);
      travel(std::nanf(""), 0, true);
    }
    gen_->end_task_script_block();
    // 0002 pure prespray task
    gen_->start_task_script_block("xMIN", "0002");
    setTravelSpeed(config_.prespray_travel_speed);
    if (is_rotary_task_) {
      travel(x, std::nanf(""), true, 0);
      travel(std::nanf(""), y, true, 0);
      moveZ(35);
    } else {
      travel(x, y, true, 0);
    }
    gen_->enter_printer_mode();
    gen_->start_printer_packet(17);
    gen_->write_printer_packet(prespray_payload);
    gen_->end_printer_packet();
    writeSimpleFilledTaskCode(prespray_bbox);
    setTravelSpeed(config_.travel_speed);
    gen_->wait_printer_mode_sync();
    gen_->exit_printer_mode();
    moveto(config_.travel_speed, std::nanf(""), std::nanf(""), std::nanf(""), 0);
    if (is_rotary_task_) {
      moveZ(1);
      travel(std::nanf(""), 0, true);
    }
    gen_->end_task_script_block();
  }
  if (canceled) {
    return false;
  }

  // Step 6. Handle post-task
  if (is_v2_) {
    gen_->start_task_script_block("xMIN", "0004");
  }
  if (is_rotary_task_) {
    if (is_v2_) {
      moveZ(1);
      travel(std::nanf(""), config_.spinning_axis_coord);
      travel(std::nanf(""), 0, true);
      gen_->sync_grbl_motion(36);
      is_a_mode_ = false;
    } else {
      travel(0, config_.spinning_axis_coord);
    }
    if (rotary_y_ratio_ != 1) {
      rotary_y_ratio_ = 1;
    }
  }
  travel(0, 0);
  if (is_v2_) {
    if (is_rotary_task_) {
      gen_->sync_motion_type2(185, 128, 0.0);
    } else {
      gen_->sync_motion_type2(179, 128, 3.0);
    }
    gen_->end_task_script_block();
    gen_->end_content();
    gen_->write_post_config(post_config);
  }
  gen_->terminated();
  qInfo() << "[Export] Took " << t.elapsed() << " milliseconds";
  return true;
}

void ToolpathExporterFcode::updateLayerParam() {
  layer_module_ = with_module_ ? current_layer_->module() : 15; // 15 = UNIVERSAL_LASER
  is_printing_layer_ = layer_module_ == 5; // 5 = PRINTER
  layer_color_ = current_layer_->color().name();
  focus_adjust_ = current_layer_->focus();
  focus_step_ = current_layer_->focusStep();
  pwm_scale_ = 1 - current_layer_->minPower() / current_layer_->power();
  if (pwm_scale_ <= 0) {
    pwm_scale_ = 1;
  }
  float min_padding;
  if (is_printing_layer_) {
    has_focus_adjust_ = false;
    enable_bidirection_ = !config_.is_one_way_printing;
    layer_speed_sec_ = qMax(float(current_layer_->printingSpeed()), config_.min_speed);
    min_padding = config_.min_printing_padding;
    // Update submodule color
    if (layer_color_ == "#9FE3FF" || layer_color_ == "#009FE3") {
      submodule_color_ = "cyan";
    } else if (layer_color_ == "#E6007E") {
      submodule_color_ = "magenta";
    } else if (layer_color_ == "#FFED00") {
      submodule_color_ = "yellow";
    } else if (layer_color_ == "#E2E2E2") {
      submodule_color_ = "white";
    } else {
      submodule_color_ = "black";
    }
  } else {
    has_focus_adjust_ = focus_adjust_ > 0;
    enable_bidirection_ =
        !(config_.enable_diode && current_layer_->isUseDiode() &&
          config_.is_diode_one_way_engraving);
    layer_speed_sec_ = qMax(float(current_layer_->speed()), config_.min_speed);
    min_padding = config_.min_engraving_padding;
    submodule_color_ = "None";
  }
  // Update padding
  if (std::isnan(min_padding)) {
    if (hardware_ == ToolpathExporterFcode::HardwareType::Ador) {
      switch (layer_module_) {
        case 1:
          min_padding = 15;
          break;
        case 2:
        case 4:
          min_padding = 25;
          break;
        case 5:
        default:
          min_padding = 10;
          break;
      }
    } else {
      min_padding = 0;
    }
  }
  float braking_distance = (pow(layer_speed_sec_, 2) / (2 * config_.padding_acc));
  padding_mm_ = qMax(qMax(min_padding, braking_distance), float(0.0));
  padding_px_ = qFloor(mm2px(padding_mm_, true));
  qInfo() << "Layer padding: " << padding_mm_ << "mm, " << padding_px_ << "px";
  // Update backlash
  if (config_.enable_custom_backlash) {
    backlash_ = current_layer_->xBacklash();
  } else if (hardware_ == ToolpathExporterFcode::HardwareType::Ador) {
    if (layer_speed_sec_ < 75) {
      backlash_ = 0;
    } else if (layer_speed_sec_ < 150) {
      backlash_ = 0.1;
    } else if (layer_speed_sec_ < 225) {
      backlash_ = 0.2;
    } else if (layer_speed_sec_ < 325) {
      backlash_ = 0.3;
    } else {
      backlash_ = 0.4;
    }
  }
  // Update speed in min
  layer_speed_ = layer_speed_sec_ * 60;
  path_speed_ = layer_speed_;
  if (config_.enable_vector_speed_constraint && path_speed_ > 1200) {
    path_speed_ = 1200;
  }
  // Update offset
  updateOffset();
  // Update clip
  updateClip();
}

void ToolpathExporterFcode::updateOffset() {
  if (with_module_) {
    module_offset_ = config_.module_offsets[layer_module_];
  } else if (config_.enable_diode && current_layer_->isUseDiode()) {
    module_offset_ = config_.diode_offset;
  } else {
    module_offset_ = QPointF(0, 0);
  }
}

void ToolpathExporterFcode::updateClip() {
  float layer_clip[4];
  qreal module_clip[4] = {0, 0, 0, 0};
  if (with_module_) {
    if (hardware_ == ToolpathExporterFcode::HardwareType::Ador) {
      // module boundary
      switch (layer_module_) {
        case 1:
          module_clip[2] = 20;
          break;
        case 2:
          module_clip[2] = 30;
          break;
        case 4:
          module_clip[0] = 26.95, module_clip[2] = 38;
          break;
        case 5:
          module_clip[2] = 50;
          break;
        default:
          break;
      }
    }
    if (module_offset_.x() > 0) {
      module_clip[3] = qMax(module_clip[3], module_offset_.x());
    } else {
      module_clip[1] = qMax(module_clip[1], -module_offset_.x());
    }
    if (is_rotary_task_) {
      module_clip[0] = 0, module_clip[2] = 0;
    } else if (module_offset_.y() > 0) {
      module_clip[0] = qMax(module_clip[0], module_offset_.y());
      module_clip[2] = qMax(module_clip[2] - module_offset_.y(), 0.0);
    } else {
      module_clip[2] = qMax(module_clip[2], -module_offset_.y());
    }
  }
  for (int i = 0; i < 4; i++) {
    layer_clip[i] = qMax((config_.workarea_clip[i]), float(module_clip[i]));
  }

  int clip_top = mm2px(layer_clip[0]);
  int clip_right = mm2px(work_area_mm_.width() - layer_clip[1], true) - 1;
  int clip_bottom = mm2px(work_area_mm_.height() - layer_clip[2]) - 1;
  int clip_left = mm2px(layer_clip[3], true);
  clip_area_ = QRect(QPoint(clip_left, clip_top), QPoint(clip_right, clip_bottom));
  border_lines_[0] = QLineF(clip_left, clip_top, clip_right, clip_top);
  border_lines_[1] = QLineF(clip_right, clip_top, clip_right, clip_bottom);
  border_lines_[2] = QLineF(clip_left, clip_bottom, clip_right, clip_bottom);
  border_lines_[3] = QLineF(clip_left, clip_top, clip_left, clip_bottom);
}

void ToolpathExporterFcode::convertLaserLayer() {
  setTransform();
  polygons_mutex_.lock();
  layer_polygons_.clear();
  layer_bitmaps_.clear();
  polygons_mutex_.unlock();
  is_handling_bitmap_ = false;
  layer_painter_ = std::make_unique<QPainter>(&laser_bitmap_);
  laser_bitmap_.fill(Qt::white);
  preview_painter_ = std::make_unique<QPainter>(&preview_bitmap_);
  preview_bitmap_.fill(Qt::white);
  bitmap_dirty_area_ = QRectF();
  // First pass: Generate path list and bitmap list and draw filled path by layer_painter_
  for (auto& shape : current_layer_->children()) {
    convertShape(shape);
  }
  // Reverse order and handle closed path
  sortPolygons();
  // Part 1: Generate path fcode
  outputLayerPathFcode();

  is_handling_bitmap_ = true;
  // Part 2: Generate filled path fcode
  outputBitmapFcode();

  // Second pass
  // Part 3: Generate bitmap fcode
  for (auto& shape : layer_bitmaps_) {
    // Note: Bitmap list is already reversed for first-depth shapes
    // When converting group, reverse order of children and ignore paths
    convertShape(shape);
  }
  layer_painter_->end();
  preview_painter_->end();
}

void ToolpathExporterFcode::convertPrintingLayer() {
  gen_->enter_printer_mode();

  float black_ratio = 1;
  if (config_.enable_multipass_compensation) {
    float ink = float(current_layer_->ink());
    int multipass = current_layer_->multipass();
    int actual_saturation = qMin(int(std::ceil(ink / multipass)), 9);
    black_ratio = qMin(ink / multipass / actual_saturation, float(1.0));
  }
  float halftone_multiplier = current_layer_->printingStrength() / 100 * black_ratio;

  QByteArray nozzle_settings_payload = generateNozzleSettingPayload();
  gen_->start_printer_packet(17);
  gen_->write_printer_packet(nozzle_settings_payload);
  gen_->end_printer_packet();

  // Add image
  setTransform();
  layer_painter_ = std::make_unique<QPainter>(&printing_bitmap_);
  // Explicitly set clip area for fm dithering
  layer_painter_->setClipRect(clip_area_);
  printing_bitmap_.fill(Qt::white);
  preview_painter_ = std::make_unique<QPainter>(&preview_bitmap_);
  preview_bitmap_.fill(Qt::white);
  bitmap_dirty_area_ = QRectF();
  for (auto& shape : current_layer_->children()) {
    convertShape(shape);
  }

  // Generate bitmap
  outputLayerPrintingFcode(halftone_multiplier);

  layer_painter_->end();
  preview_painter_->end();

  gen_->exit_printer_mode();
}

bool ToolpathExporterFcode::convertShape(const ShapePtr& shape,
                                         bool from_group) {
  const PathShape* path;
  // Add bitmap or group with bitmap to layer_bitmaps_
  bool has_bitmap = false;
  switch (shape->type()) {
    case Shape::Type::Group:
      has_bitmap = convertGroup(dynamic_cast<GroupShape*>(shape.get()));
      if (has_bitmap && !(is_printing_layer_ || is_handling_bitmap_ || from_group)) {
        layer_bitmaps_.prepend(shape);
      }
      break;
    case Shape::Type::Bitmap:
      has_bitmap = true;
      if (is_printing_layer_ || is_handling_bitmap_) {
        convertBitmap(dynamic_cast<BitmapShape*>(shape.get()));
      } else if (!from_group) {
        layer_bitmaps_.prepend(shape);
      }
      break;
    case Shape::Type::Path:
    case Shape::Type::Text:
      if (!is_handling_bitmap_) {
        convertPath(dynamic_cast<PathShape*>(shape.get()));
      }
      break;
    default:
      break;
  }
  return has_bitmap;
}

bool ToolpathExporterFcode::convertGroup(const GroupShape* group) {
  bool has_filled = false;
  setTransform(group->globalTransform());
  if (is_handling_bitmap_) {
    for (auto shape_rit = group->children().crbegin(); shape_rit != group->children().crend(); shape_rit++) {
      convertShape(*shape_rit, true);
    }
  } else {
    for (auto& shape : group->children()) {
      has_filled |= convertShape(shape, true);
    }
  }
  setTransform();
  return has_filled;
}

void ToolpathExporterFcode::convertBitmap(const BitmapShape* bmp) {
  QRectF new_dirty_area = global_transform_.mapRect(bmp->boundingRect());
  QTransform transform = bmp->transform() * global_transform_;
  QImage transformed_image =
      bmp->sourceImage()
          .transformed(transform, Qt::SmoothTransformation)
          .convertToFormat(QImage::Format_ARGB32);
  if (bmp->gradient()) {
    if (is_v2_) {
      preview_painter_->save();
      preview_painter_->setTransform(getPreviewTransform(), false);
      preview_painter_->drawImage(new_dirty_area.topLeft(), transformed_image);
      preview_painter_->restore();
    }
    if (bmp->pwm() || is_printing_layer_) {
      layer_painter_->drawImage(new_dirty_area.topLeft(), transformed_image);
    } else {
      clearTransparent(&transformed_image);
      ImageSharpenDialog sharpener = ImageSharpenDialog();
      sharpener.loadImage(transformed_image);
      sharpener.onSharpnessChanged(1);
      sharpener.onRadiusChanged(2);
      QImage image = sharpener.getSharpenedImage()
                         .convertToFormat(QImage::Format_Mono,
                                          Qt::MonoOnly | Qt::DiffuseAlphaDither)
                         .convertToFormat(QImage::Format_Grayscale8);
      layer_painter_->drawImage(new_dirty_area.topLeft(), image);
    }
  } else {
    QImage image = imageBinarize(&transformed_image, bmp->thrsh_brightness());
    if (is_v2_) {
      preview_painter_->save();
      preview_painter_->setTransform(getPreviewTransform(), false);
      preview_painter_->drawImage(new_dirty_area.topLeft(), image);
      preview_painter_->restore();
    }
    layer_painter_->drawImage(new_dirty_area.topLeft(), image);
  }
  if (is_printing_layer_) {
    bitmap_dirty_area_ = bitmap_dirty_area_.united(new_dirty_area);
  } else {
    bitmap_dirty_area_ = new_dirty_area;
    outputBitmapFcode(config_.enable_pwm && bmp->pwm());
  }
}

void ToolpathExporterFcode::convertPath(const PathShape* path) {
  QPainterPath transformed_path =
      (path->transform() * global_transform_).map(path->path());
  QRectF path_bounding_rect = transformed_path.boundingRect();

  bool has_filled =
      ((path->isFilled() && current_layer_->type() == Layer::Type::Mixed) ||
       current_layer_->type() == Layer::Type::Fill ||
       current_layer_->type() == Layer::Type::FillLine);
  if (has_filled) {
    layer_painter_->setPen(Qt::NoPen);
    layer_painter_->setBrush(Qt::black);
    layer_painter_->drawPath(transformed_path);
    layer_painter_->setBrush(Qt::NoBrush);
    if (is_v2_) {
      preview_painter_->setPen(Qt::NoPen);
      preview_painter_->setBrush(Qt::black);
      preview_painter_->drawPath(transformed_path * getPreviewTransform());
      preview_painter_->setBrush(Qt::NoBrush);
    }
    bitmap_dirty_area_ = bitmap_dirty_area_.united(path_bounding_rect);
  } else if (is_printing_layer_) {
    // Note: This is for dev convinience
    // Path in BVG input should already been converted to image
    layer_painter_->drawPath(transformed_path);
    if (is_v2_) {
      preview_painter_->drawPath(transformed_path * getPreviewTransform());
    }
    bitmap_dirty_area_ = bitmap_dirty_area_.united(path_bounding_rect);
  } else if (path_bounding_rect.left() > clip_area_.right() ||
             path_bounding_rect.right() < clip_area_.left() ||
             path_bounding_rect.top() > clip_area_.bottom() ||
             path_bounding_rect.bottom() < clip_area_.top()) {
    qInfo() << "No point in clip area; skip path";
  } else {
    polygons_mutex_.lock();
    layer_polygons_.append(transformed_path.toSubpathPolygons());
    if (is_v2_) {
      preview_painter_->drawPath(transformed_path * getPreviewTransform());
    }
    polygons_mutex_.unlock();
  }
}

// Complexity: O(n^2)
void ToolpathExporterFcode::sortPolygons() {
  QList<QPolygonF> sort_result;
  QVector2D unit(1 / dpmm_x(), 1 / dpmm_y());
  for (const auto& polygon : layer_polygons_) {
    // Insert from the beginning (reverse order)
    int insert_idx = 0;
    float d = (QVector2D(polygon.first() - polygon.last()) * unit).length();
    if (d <= config_.loop_compensation) {
      // Handle closed path
      for (int idx = sort_result.size() - 1; idx >= 0; idx--) {
        if (polygon.boundingRect().contains(sort_result[idx].boundingRect())) {
          // Move current polygon after smaller one
          insert_idx = idx + 1;
          break;
        }
      }
    }
    sort_result.insert(insert_idx, polygon);
  }
  layer_polygons_ = sort_result;
}

void ToolpathExporterFcode::outputLayerPathFcode() {
  bool should_set_acc = !isnan(config_.path_acc) && is_v2_;
  polygons_mutex_.lock();
  for (auto& poly : layer_polygons_) {
    if (poly.empty()) {
      continue;
    }
    if (should_set_acc) {
      setPathAcceleration(config_.path_acc, config_.path_acc);
    }
    setTravelSpeed(config_.path_travel_speed);
    int last_position = getPointPosition(poly.first());
    if (last_position == 4) {
      handlePathWalk(poly.first(), false);
    }
    QPointF* last_point = nullptr;
    for (QPointF& point : poly) {
      int position = getPointPosition(point);
      QPointF intersection;
      if (position == 4) {
        if (last_position != 4) {
          // From outside to inside
          // Handle additional intersection point first and start laser
          QLineF current_line(*last_point, point);
          getIntersectPoint(current_line, last_position, &intersection);
          handlePathWalk(intersection, false);
          handlePathWalk(intersection, true);
        }
        // Normal walk
        handlePathWalk(point, true);
      } else if (last_position == 4) {
        // From inside to outside
        // Clip current point and stop laser
        QLineF current_line(*last_point, point);
        getIntersectPoint(current_line, position, &intersection);
        handlePathWalk(intersection, true);
        handlePathWalk(intersection, false);
      } else if (isIntersectWithCenter(last_position, position)) {
        // From one side to another side
        // Need to handle two intersection points
        QLineF current_line(*last_point, point);
        getIntersectPoint(current_line, last_position, &intersection);
        handlePathWalk(intersection, false);
        handlePathWalk(intersection, true);
        getIntersectPoint(current_line, position, &intersection);
        handlePathWalk(intersection, true);
        handlePathWalk(intersection, false);
      }
      last_point = &point;
      last_position = position;
    }
    if (last_position == 4) {
      handlePathWalk(*last_point, false);
    }
    setTravelSpeed(config_.travel_speed);
    // Reset path_acc
    if (should_set_acc) {
      gen_->sync_grbl_motion(151);
      gen_->set_time_est_acc(config_.padding_acc);
    }
  }
  polygons_mutex_.unlock();
}

int ToolpathExporterFcode::getPointPosition(QPointF point) {
  int position = 4;
  if (point.x() < clip_area_.left()) {
    position -= 1;
  } else if (point.x() > clip_area_.right()) {
    position += 1;
  }
  if (point.y() < clip_area_.top()) {
    position -= 3;
  } else if (point.y() > clip_area_.bottom()) {
    position += 3;
  }
  return position;
}

void ToolpathExporterFcode::getIntersectPoint(QLineF line,
                                              int position,
                                              QPointF* point) {
  QLineF::IntersectionType type;
  switch (position) {
    case 0:
      type = line.intersects(border_lines_[0], point);
      if (type != QLineF::BoundedIntersection) {
        line.intersects(border_lines_[3], point);
      }
      break;
    case 1:
      line.intersects(border_lines_[0], point);
      break;
    case 2:
      type = line.intersects(border_lines_[0], point);
      if (type != QLineF::BoundedIntersection) {
        line.intersects(border_lines_[1], point);
      }
      break;
    case 3:
      line.intersects(border_lines_[3], point);
      break;
    case 5:
      line.intersects(border_lines_[1], point);
      break;
    case 6:
      type = line.intersects(border_lines_[2], point);
      if (type != QLineF::BoundedIntersection) {
        line.intersects(border_lines_[3], point);
      }
      break;
    case 7:
      line.intersects(border_lines_[2], point);
      break;
    case 8:
      type = line.intersects(border_lines_[2], point);
      if (type != QLineF::BoundedIntersection) {
        line.intersects(border_lines_[1], point);
      }
      break;
    default:
      break;
  }
}

void ToolpathExporterFcode::handlePathWalk(QPointF point, bool should_emit) {
  QPointF next_point_mm = getPointInMM(point);
  if (with_blade_) {
    QVector2D target_vector(next_point_mm - current_xy_);
    if (!target_vector.isNull()) {
      if (!current_vector_.isNull()) {
        float angle = getAngle(current_vector_, target_vector);
        while (abs(angle) > 0.01) {
          int dir = angle > 0 ? 1 : -1;
          float rotate_angle = dir * qMin(abs(angle), float(0.1));
          QVector2D rotated_vector(current_vector_.x() * cos(rotate_angle) -
                                       current_vector_.y() * sin(rotate_angle),
                                   current_vector_.x() * sin(rotate_angle) +
                                       current_vector_.y() * cos(rotate_angle));
          rotated_vector.normalize();
          rotated_vector *= config_.blade_radius;
          moveto(300, current_xy_.x() + rotated_vector.x(),
                 current_xy_.y() + rotated_vector.y());
          angle = getAngle(rotated_vector, target_vector);
        }
      }
      if (gen_->current_pwm != 0) {
        current_vector_ = target_vector;
      }
    }
  }
  if (with_blade_ && !current_vector_.isNull()) {
    next_point_mm = getBladeCompensation(next_point_mm, current_vector_,
                                         config_.blade_radius);
    moveto(path_speed_, next_point_mm.x(), next_point_mm.y());
  } else if (should_emit) {
    moveto(path_speed_, next_point_mm.x(), next_point_mm.y());
  } else {
    travel(next_point_mm);
  }
  current_xy_ = next_point_mm;
  float target_power = should_emit ? 100 : 0;
  if (gen_->current_pwm != target_power) {
    gen_->set_toolhead_pwm(target_power, true);
  }
}

// Handling bitmap and filled path
void ToolpathExporterFcode::outputBitmapFcode(bool pwm_engraving) {
  if (bitmap_dirty_area_.width() == 0) {
    qInfo() << "Skip: empty bitmap";
  } else {
    QImage layer_image = laser_bitmap_.toImage().convertToFormat(QImage::Format_Grayscale8);
    QVector<QRect> bboxes = getBoundingBoxes(&layer_image, padding_px_, 5);
    char gradient_print_mode = 0;
    if (config_.enable_fast_gradient) {
      gradient_print_mode = pwm_engraving ? config_.print_modes[0] : config_.print_modes[1];
    }
    for (auto bbox : bboxes) {
      if (gradient_print_mode != 0) {
        gen_->turn_on_gradient_print_mode(gradient_print_mode);
      }
      QPointF start = getPointInMM(bbox.topLeft());
      travel(start);
      int step = bbox.width();
      if (pwm_engraving && config_.fg_pwm_limit) {
        step = qMax(config_.fg_pwm_limit - padding_px_ * 2, 100);
      }
      for (int left = bbox.left(); left <= bbox.right(); left += step) {
        int right = qMin(left + step, bbox.left() + bbox.width()) - 1;
        QRect sliced_box = QRect(bbox);
        sliced_box.setLeft(left);
        sliced_box.setRight(right);
        rasterBitmap(layer_image, sliced_box, pwm_engraving);
      }
      if (config_.enable_fast_gradient) {
        gen_->turn_off_gradient_print_mode();
      }
    }
  }

  // Clear canvas
  laser_bitmap_.fill(Qt::white);
  bitmap_dirty_area_ = QRectF();
}

bool ToolpathExporterFcode::rasterBitmap(const QImage& layer_image,
                                         QRect bbox,
                                         bool pwm_engraving) {
  bool reverse_raster_dir = false;
  int left = bbox.left();
  int right = bbox.right();
  int y_top = bbox.top();
  int y_bottom = bbox.bottom();
  int y = config_.is_reverse_engraving ? y_bottom : y_top;
  int y_step = config_.is_reverse_engraving ? -1 : 1;
  bool has_move = false;

  while (true) {
    if (config_.is_reverse_engraving) {
      if (y < y_top) {
        break;
      }
    } else if (y > y_bottom) {
      break;
    }

    const uchar* data_ptr = layer_image.constScanLine(y);
    if (config_.enable_fast_gradient) {
      if (pwm_engraving) {
        has_move = rasterLineHighSpeedPwm(data_ptr, left, right, y, reverse_raster_dir);
      } else {
        has_move = rasterLineHighSpeed(data_ptr, left, right, y, reverse_raster_dir);
      }
    } else {
      has_move = rasterLine(data_ptr, left, right, y, reverse_raster_dir);
    }
    if (has_move && enable_bidirection_) {
      reverse_raster_dir = !reverse_raster_dir;
    }

    y += y_step;
  }
  return true;
}

bool ToolpathExporterFcode::rasterLine(const uchar* data_ptr,
                                       int left_bound,   // included
                                       int right_bound,  // included
                                       int y,
                                       bool reverse_raster_dir) {
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

    if (data_ptr[x] < white_val) {
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
          moveto(layer_speed_ + 1, std::nanf(""), getYValInMM(y));
          moveto(layer_speed_);
        }
        current_x = x;
        moveto(std::nanf(""), getXValInMM(current_x, reverse_raster_dir));
        has_unfinished_move = false;
        gen_->set_toolhead_pwm(100);
        is_emitting = true;
      }
    } else if (is_emitting) {
      // Laser on -> off
      current_x = x;
      moveto(std::nanf(""), getXValInMM(current_x, reverse_raster_dir));
      has_unfinished_move = false;
      gen_->set_toolhead_pwm(0);
      is_emitting = false;
    }

    x += x_step;
  }

  if (current_x >= 0) {
    qreal real_x = getXValInMM(current_x, reverse_raster_dir);
    if (has_unfinished_move) {
      moveto(std::nanf(""), real_x);
    }
    qreal laser_padding = 25;
    if (config_.enable_mock_fast_gradient) {
      laser_padding = padding_mm_;
    }
    qreal buffer_x;
    if (reverse_raster_dir) {
      buffer_x = qMax(real_x - laser_padding, 0.0);
    } else {
      buffer_x = qMin(real_x + laser_padding, work_area_mm_.width());
    }
    gen_->set_toolhead_pwm(0);
    moveto(std::nanf(""), buffer_x);
    return true;
  } else {
    // Blank line
    return false;
  }
}

bool ToolpathExporterFcode::rasterLineHighSpeed(const uchar* data_ptr,
                                                int left_bound,   // included
                                                int right_bound,  // included
                                                int y,
                                                bool reverse_raster_dir) {
  bool is_emitting = false;
  bool should_emit;
  // Note: left and right are both emitting points
  int left = left_bound;
  int right = right_bound;

  // Find the left-most emitting point
  for (; left <= right_bound; left++) {
    if (data_ptr[left] < white_val) {
      break;
    }
  }
  // Skip blank line
  if (left > right_bound) {
    return false;
  }
  // Find the right-most emitting point
  for (; right >= left; right--) {
    if (data_ptr[right] < white_val) {
      break;
    }
  }
  left = qMax(left_bound, left - padding_px_);
  right = qMin(right_bound, right + padding_px_);
  float buffer_left = getXValInMM(left, reverse_raster_dir, false);
  float buffer_right = getXValInMM(right + 1, reverse_raster_dir, false);
  moveto(layer_speed_ + 1, reverse_raster_dir ? buffer_right : buffer_left, getYValInMM(y));
  moveto(layer_speed_);
  gen_->set_line_pixels(right + 1 - left);
  // 32 bits data, 1 bit per pixel (0 or 1)
  uint32_t current_val = 0;
  int bit_id = 31;

  if (reverse_raster_dir) {
    for (int x = right; x >= left; x--) {
      if (data_ptr[x] < white_val) {
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
      if (data_ptr[x] < white_val) {
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
  moveto(std::nanf(""), reverse_raster_dir ? buffer_left : buffer_right);
  return true;
}

bool ToolpathExporterFcode::rasterLineHighSpeedPwm(const uchar* data_ptr,
                                                   int left_bound,   // included
                                                   int right_bound,  // included
                                                   int y,
                                                   bool reverse_raster_dir) {
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
  float buffer_left = getXValInMM(left, reverse_raster_dir, false);
  float buffer_right = getXValInMM(right + 1, reverse_raster_dir, false);
  moveto(layer_speed_, reverse_raster_dir ? buffer_right : buffer_left, getYValInMM(y));
  gen_->set_line_pixels(right + 1 - left);
  // 32 bits data, 8 bits per pixel (0~255)
  uint32_t current_val = 0;
  int bit_offset = 24;
  uint32_t val;

  if (reverse_raster_dir) {
    for (int x = right; x >= left; x--) {
      if (x >= left_bound && x <= right_bound && data_ptr[x] < pwm_threshold) {
        val = std::round(white_val - data_ptr[x] * pwm_scale_);
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
        val = std::round(white_val - data_ptr[x] * pwm_scale_);
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
  moveto(std::nanf(""), reverse_raster_dir ? buffer_left : buffer_right);
  return true;
}

void ToolpathExporterFcode::outputLayerPrintingFcode(float halftone_multiplier) {
  if (!bitmap_dirty_area_.isValid()) {
    return;
  }
  int bbox_left = qMax(qRound(bitmap_dirty_area_.left() - padding_px_), clip_area_.left());
  int bbox_top = qMax(qRound(bitmap_dirty_area_.top()), clip_area_.top());
  int bbox_right = qMin(qRound(bitmap_dirty_area_.right() + padding_px_), clip_area_.right());
  int bbox_bottom = qMin(qRound(bitmap_dirty_area_.bottom()), clip_area_.bottom());
  if (bbox_left > bbox_right || bbox_top > bbox_bottom) {
    return;
  }

  QImage layer_image = printing_bitmap_.toImage().convertToFormat(QImage::Format_Grayscale8);
  QImage val_table{layer_image.size(), QImage::Format_Grayscale8};
  val_table.fill(0);
  bool do_color_curve = submodule_color_ != "white";
  int multipass = current_layer_->multipass();
  // 1: FM, 2: AM
  bool do_am = current_layer_->halftone() > 1;
  QList<int> color_curve = do_am ? am_color_curves[submodule_color_]
                                 : fm_color_curves[submodule_color_];
  float am_cos;
  float am_sin;
  float am_dot_r;
  float am_dot_d;
  if (do_am) {
    float rad = am_angles[submodule_color_] * M_PI / 180;
    am_cos = cos(rad);
    am_sin = sin(rad);
    am_dot_r = qMax(config_.dpmm_printing / (2.0 * am_density), 1.0);
    am_dot_d = am_dot_r * 2;
  }

  // Preprocess image
  for (int y = bbox_top; y <= bbox_bottom; y++) {
    uchar* data_ptr = layer_image.scanLine(y);
    for (int x = bbox_left; x <= bbox_right; x++) {
      int inv_val = white_val - data_ptr[x];
      if (inv_val == 0) {
        // Skip white pixels
        continue;
      }
      // Color curve
      if (do_color_curve) {
        int k = 64;
        int q = inv_val / k;
        int r = inv_val % k;
        inv_val = int(float(color_curve[q] * (k - r) + color_curve[q + 1] * r) / k);
      }
      // Halftone
      if (do_am) {
        float dot_size =
            am_dot_r * (pow(float(inv_val) / white_val, halftone_smoother) *
                        halftone_multiplier * 1.414);
        float dx = positiveMod((x * am_cos - y * am_sin + am_dot_r), am_dot_d) - am_dot_r;
        float dy = positiveMod((x * am_sin + y * am_cos + am_dot_r), am_dot_d) - am_dot_r;
        float d = pow(pow(dx, 2) + pow(dy, 2), 0.5);
        if (d <= dot_size) {
          inv_val = white_val;
        } else {
          inv_val = 0;
        }
      } else {
        inv_val = qMin(int(pow(float(inv_val) / white_val, halftone_smoother) *
                           halftone_multiplier * white_val),
                       white_val);
      }
      data_ptr[x] = (uchar)(white_val - inv_val);
    }
  }
  if (!do_am) {
    layer_image.convertTo(QImage::Format_Mono, Qt::DiffuseDither);
    layer_image.convertTo(QImage::Format_Grayscale8);
  }
  int padded_top = qMax(bbox_top - 2 * printing_slice_height, 0);
  int padded_bottom = qMin(bbox_bottom + 2 * printing_slice_height, layer_image.height());
  const uchar* last_val_ptr = val_table.constScanLine(0);
  for (int y = padded_top; y <= padded_bottom; y++) {
    const uchar* data_ptr = layer_image.constScanLine(y);
    uchar* val_ptr = val_table.scanLine(y);
    for (int x = bbox_left; x < bbox_right; x++) {
      uchar val = data_ptr[x] == white_val ? 0 : 0b10000000;
      uchar prev_val = last_val_ptr[x];
      if ((prev_val | val) == 0) {
        // All 9 involving pixels are white; keep val = 0
        continue;
      }
      val_ptr[x] = prev_val >> 1 | val;
    }
    last_val_ptr = val_ptr;
  }

  // Slice boxes for each contour
  QVector<QRect> bboxes = getBoundingBoxes(&layer_image, padding_px_, printing_slice_height);
  QList<QList<QList<int>>> sliced_boxes = {};
  for (auto bbox : bboxes) {
    sliceBox(&sliced_boxes, bbox, multipass);
  }

  // Generate fcode
  int repeat = current_layer_->repeat();
  for (auto& boxes : sliced_boxes) {
    bool reverse_raster_dir = false;
    for (auto box = boxes.cbegin(); box != boxes.cend(); box++) {
      int left_x = -1;
      int right_x = -1;  // included
      int padded_w = -1;
      int box_left = box->at(0);
      int box_top = box->at(1);
      QByteArray payload[2];
      QVector<QByteArray> payload_data;
      int pixel_count = 0;
      for (int i = 0; i < repeat; i++) {
        int idx = reverse_raster_dir ? 1 : 0;
        if (pixel_count == 0) {
          // Generate payload
          int box_right = box_left + box->at(2);
          int box_bottom = box_top + box->at(3);
          int min_valid_y = box_top + box->at(4);
          for (int x = box_left; x < box_right; x++) {
            int column_count = 0;
            QByteArray column_payload;
            for (int y = box_top + printing_slice_height; y >= box_top;
                 y -= 8) {
              if (y > box_bottom + 8 || y < 0 || y < min_valid_y) {
                column_payload.append((char)0);
              } else if (y >= box_bottom) {
                int overflow = y - box_bottom + 1;
                uchar val =
                    val_table.constScanLine(box_bottom - 1)[x] >> overflow;
                column_payload.append(val);
                column_count += ((std::bitset<8>)val).count();
              } else if (y - 7 < min_valid_y) {
                uchar val = val_table.constScanLine(box_bottom - 1)[x];
                // Note: y >= min_valid_y
                int valid = y - min_valid_y + 1;
                int overflow = 8 - valid;
                val = val >> overflow << overflow;
                column_payload.append(val);
                column_count += ((std::bitset<8>)val).count();
              } else {
                uchar val = val_table.constScanLine(y - 1)[x];
                column_payload.append(val);
                column_count += ((std::bitset<8>)val).count();
              }
            }
            if (column_count > 0) {
              if (left_x < 0) {
                left_x = x;
              }
              right_x = x;
              pixel_count += column_count;
            }
            payload_data.append(column_payload);
          }
          if (pixel_count == 0) {
            // Skip empty box
            break;
          }
          left_x = qMax(left_x - padding_px_, box_left);
          right_x = qMin(right_x + padding_px_, box_right - 1);
          padded_w = right_x - left_x + 1;
        }
        if (payload[idx].isEmpty()) {
          payload[idx].append((const char*)(&padded_w), 4);
          payload[idx].append((const char*)(&printing_slice_height), 4);
          payload[idx].append(12, (char)0); // For x,y,reserved
          if (reverse_raster_dir) {
            for (int x_id = (right_x - box_left); x_id >= (left_x - box_left); x_id--) {
              payload[idx].append(payload_data[x_id]);
            }
          } else {
            for (int x_id = (left_x - box_left); x_id <= (right_x - box_left); x_id++) {
              payload[idx].append(payload_data[x_id]);
            }
          }
        }
        // Note: don't pass reverse_raster_dir
        // No need to fix side offset and backlash
        float real_x = getXValInMM(reverse_raster_dir ? right_x : left_x);
        float real_y = getYValInMM(box_top);
        travel(real_x, real_y, false, 0);
        gen_->start_printer_packet(2);
        gen_->write_printer_packet(payload[idx]);
        gen_->wait_printer_mode_sync();
        gen_->end_printer_packet();
        gen_->set_printer_packet_px_count(pixel_count);
        real_x = getXValInMM(reverse_raster_dir ? left_x : right_x);
        moveto(layer_speed_, real_x, real_y, std::nanf(""), 1);

        if (enable_bidirection_) {
          reverse_raster_dir = !reverse_raster_dir;
        }
      }
    }
  }
}

QByteArray ToolpathExporterFcode::generateNozzleSettingPayload(
    int saturation,
    bool use_default) {
  QByteArray payload;
  if (use_default) {
    payload.append((const char*)(&nozzle_settings.voltage_default), 4);
    payload.append((const char*)(&nozzle_settings.pulse_width_default), 4);
  } else {
    payload.append((const char*)(&nozzle_settings.voltage), 4);
    payload.append((const char*)(&nozzle_settings.pulse_width), 4);
  }
  payload.append((const char*)(&saturation), 4);
  payload.append((const char*)(&nozzle_settings.DPI), 4);
  payload.append((const char*)(&nozzle_settings.ink_catridge_count), 4);
  payload.append((const char*)(&nozzle_settings.ink_type), 4);
  payload.append((const char*)(&nozzle_settings.nozzle_select), 4);
  payload.append((const char*)(&nozzle_settings.spray_time), 4);
  payload.append((const char*)(&nozzle_settings.ink_exchange), 4);
  payload.append((const char*)(&nozzle_settings.h_gap_ink1_ink2), 4);
  payload.append((const char*)(&nozzle_settings.v_gap_ink1_ink2), 4);
  payload.append((const char*)(&nozzle_settings.h_gap_ink2_ink3), 4);
  payload.append((const char*)(&nozzle_settings.v_gap_ink2_ink3), 4);
  payload.append((const char*)(&nozzle_settings.h_gap_ink3_ink4), 4);
  payload.append((const char*)(&nozzle_settings.v_gap_ink3_ink4), 4);
  return payload;
}

void ToolpathExporterFcode::sliceBox(QList<QList<QList<int>>>* sliced_boxes,
                                     QRect box,
                                     int multipass) {
  QList<QList<int>> boxes;
  int box_left = box.x();
  int box_top = box.y();
  int box_right = box_left + box.width();
  int box_bottom = box_top + box.height();
  int slice_height = printing_slice_height - config_.printing_bot_padding;
  int real_slice_height = slice_height - config_.printing_top_padding;
  float offset_y_px = mm2px(module_offset_.y());
  int min_allow_y = std::ceil(qMin(offset_y_px, float(0.0)));
  float bottom_limit = qMax(-offset_y_px, float(0.0)) + clip_area_.bottom();
  int max_allow_y = int(printing_bitmap_.height() - bottom_limit);
  QVector<int> x_steps;
  QVector<int> y_starts(multipass);
  for (int x = box_left; x < box_right; x += printing_slice_width) {
    int w = qMin(printing_slice_width, box_right - x);
    x_steps.append(w);
  }
  for (int p = multipass - 1; p >= 0; p--) {
    int multipass_padding = ((p * real_slice_height) / multipass) + config_.printing_top_padding;
    int y_start = qMax(box_top - multipass_padding, min_allow_y);
    y_starts[p] = y_start;
  }

  int y_slice_count = (box_bottom - y_starts[multipass - 1]) / real_slice_height + 1;
  std::function<void(QList<int>)> addBox;
  if (config_.is_reverse_engraving) {
    addBox = [&boxes](QList<int> v) { boxes.prepend(v); };
  } else {
    addBox = [&boxes](QList<int> v) { boxes.append(v); };
  }

  for (int i = 0; i < y_slice_count; i++) {
    for (int p = multipass - 1; p >= 0; p--) {
      int y = y_starts[p] + i * real_slice_height;
      if (y + config_.printing_top_padding > box_bottom) {
        // Real data is out of box
        break;
      }
      int box_padding_top = config_.printing_top_padding;
      if (y >= max_allow_y) {
        box_padding_top = config_.printing_top_padding + y - max_allow_y;
        if (box_padding_top >= printing_slice_height) {
          continue;
        }
        y = max_allow_y;
      }
      int h = qMin(slice_height + box_padding_top, qMin(box_bottom - y, printing_slice_height));
      int x = box_left;
      for (int k = 0; k < x_steps.size(); k++) {
        int w = x_steps[k];
        addBox({x, y, w, h, box_padding_top});
        x += w;
      }
    }
  }
  sliced_boxes->append(boxes);
}

std::tuple<QRect, QRect> ToolpathExporterFcode::getPresprayBbox() {
  int x = std::round(mm2px(config_.prespray.x()));
  int y = std::round(mm2px(config_.prespray.y()));
  int w = std::round(mm2px(config_.prespray.width()));
  int h = std::round(mm2px(config_.prespray.height()));
  int prespray_w_px = mm2px(8);
  int x_safe_dist = std::round(mm2px(2));

  int prespray_x, prespray_y, prespray_w, prespray_h;
  int test_x, test_y, test_w, test_h;

  if (w > prespray_w_px + 2 * x_safe_dist) {
    prespray_x = x + (w - prespray_w_px) / 2;
    prespray_w = prespray_w_px;
  } else {
    prespray_x = x + x_safe_dist;
    prespray_w = w - 2 * x_safe_dist;
  }
  test_x = x + x_safe_dist;
  test_w = w - 2 * x_safe_dist;
  if (h > 2 * printing_slice_height) {
    int padding = (h - 2 * printing_slice_height) / 3;
    prespray_y = y + padding;
    prespray_h = printing_slice_height;
    test_y = y + 2 * padding + printing_slice_height;
    test_h = printing_slice_height;
  } else if (h > printing_slice_height) {
    int padding = (h - printing_slice_height) / 2;
    prespray_y = y + padding;
    prespray_h = printing_slice_height;
    test_y = y + padding;
    test_h = printing_slice_height;
  } else {
    prespray_y = y;
    prespray_h = h;
    test_y = y;
    test_h = h;
  }
  return std::make_tuple(QRect(prespray_x, prespray_y, prespray_w, prespray_h),
                         QRect(test_x, test_y, test_w, test_h));
}

// Note: -printing_slice_height < offset_y < printing_slice_height
void ToolpathExporterFcode::writeSimpleFilledTaskCode(QRect bbox,
                                                      int offset_y) {
  int box_x = bbox.x();
  int box_y = bbox.y();
  int box_w = bbox.width();
  int box_h = bbox.height();
  int column_count = qMin(box_h, printing_slice_height - abs(offset_y));
  int px_count = column_count * box_w;
  QByteArray payload;
  payload.append((const char*)(&box_w), 4);
  payload.append((const char*)(&printing_slice_height), 4);
  payload.append(12, (char)0); // For x,y,reserved
  QByteArray column_payload;

  uchar black = 0b11111111; // Eight black pixels
  int bottom_padding = printing_slice_height - box_h - offset_y;
  int count;
  int top_padding = offset_y;
  int rest;
  if (bottom_padding > 0) {
    // All White
    count = bottom_padding / 8;
    column_payload.append(count, (char)0);
    // White on bottom + Black on top
    rest = bottom_padding % 8;
    if (rest > 0) {
      uchar val = black >> rest;
      column_payload.append(val);
      column_count = column_count - 8 + rest;
    }
  }
  // All Black
  count = column_count / 8;
  column_payload.append(count, black);
  // Black on bottom + White on top
  rest = column_count % 8;
  if (rest > 0) {
    rest = 8 - rest;
    uchar val = black >> rest << rest;
    column_payload.append(val);
    top_padding = top_padding - rest;
  }
  // All White
  if (top_padding > 0) {
    count = std::ceil(float(top_padding) / 8);
    column_payload.append(count, (char)0);
  }
  count = box_w;
  while (count--) {
    payload.append(column_payload);
  }

  float real_x = getXValInMM(box_x);
  float real_y = getYValInMM(box_y - offset_y);
  travel(real_x, real_y, true, 0);
  gen_->start_printer_packet(2);
  gen_->write_printer_packet(payload);
  gen_->wait_printer_mode_sync();
  gen_->end_printer_packet();
  gen_->set_printer_packet_px_count(px_count);
  real_x = getXValInMM(box_x + box_w);
  moveto(config_.prespray_speed, real_x, real_y, std::nanf(""), 1, true);
}

void ToolpathExporterFcode::writeCatridgeTaskCode(QRect bbox) {
  int offset_interval = printing_slice_height / 3;
  int offset_y = offset_interval * 2;
  int min_offset = -bbox.height();
  while (offset_y > min_offset) {
    writeSimpleFilledTaskCode(bbox, offset_y);
    offset_y -= offset_interval;
  }
}

void ToolpathExporterFcode::writePreviewImage() {
  QImage output_image = preview_bitmap_.toImage().convertToFormat(QImage::Format_ARGB32);
  clearWhite(&output_image);
  QByteArray byteArray;
  QBuffer buffer(&byteArray);
  output_image.save(&buffer, "PNG");
  gen_->write_string("PREV", 4);
  gen_->write_string(byteArray.data(), byteArray.size(), true);
}

QImage ToolpathExporterFcode::imageBinarize(QImage* src, int threshold) {
  Q_ASSERT_X(src->allGray(), "ToolpathExporterFcode",
             "Input image for imageBinarize() must be grayscaled");
  Q_ASSERT_X(src->format() == QImage::Format_ARGB32 ||
             src->format() == QImage::Format_ARGB32_Premultiplied,
             "ToolpathExporterFcode",
             "Input image for imageBinarize() must be Format_ARGB32");

  QImage result_img{src->width(), src->height(), QImage::Format_Grayscale8};
  int black = 0;
  for (int y = 0; y < src->height(); ++y) {
    const QRgb* data_ptr = (QRgb*)src->constScanLine(y);
    uchar* result_ptr = result_img.scanLine(y);
    for (int x = 0; x < src->width(); ++x) {
      int alpha = qAlpha(data_ptr[x]);
      if (alpha == 0) {
        result_ptr[x] = white_val;
      } else {
        int gray = qGray(data_ptr[x]);
        result_ptr[x] = gray <= threshold ? black : white_val;
      }
    }
  }
  return result_img;
}

void ToolpathExporterFcode::clearWhite(QImage* src) {
  Q_ASSERT_X(src->allGray(), "ToolpathExporterFcode",
             "Input image for clearWhite() must be grayscaled");
  Q_ASSERT_X(src->format() == QImage::Format_ARGB32, "ToolpathExporterFcode",
             "Input image for clearWhite() must be Format_ARGB32");

  for (int y = 0; y < src->height(); ++y) {
    QRgb* ptr = (QRgb*)src->scanLine(y);
    for (int x = 0; x < src->width(); ++x) {
      int gray = qGray(ptr[x]);
      if (gray == white_val) {
        // Set alpha to 0
        ptr[x] = 0;
      }
    }
  }
}

void ToolpathExporterFcode::clearTransparent(QImage* src) {
  Q_ASSERT_X(src->allGray(), "ToolpathExporterFcode",
             "Input image for clearTransparent() must be grayscaled");
  Q_ASSERT_X(src->format() == QImage::Format_ARGB32, "ToolpathExporterFcode",
             "Input image for clearTransparent() must be Format_ARGB32");

  QRgb white = 0xFFFFFFFF;
  for (int y = 0; y < src->height(); ++y) {
    QRgb* ptr = (QRgb*)src->scanLine(y);
    for (int x = 0; x < src->width(); ++x) {
      if (qAlpha(ptr[x]) == 0) {
        ptr[x] = white;
      }
    }
  }
}

void ToolpathExporterFcode::pause(bool to_standby_position) {
  gen_->pause(to_standby_position);
  disable_rotary_ = to_standby_position;
}

void ToolpathExporterFcode::moveZ(float z) {
  moveto(std::nanf(""), std::nanf(""), std::nanf(""), z);
}

void ToolpathExporterFcode::travel(float x, float y, bool force_y, float s) {
  moveto(std::nanf(""), x, y, std::nanf(""), s, force_y, true);
}

void ToolpathExporterFcode::travel(QPointF position, bool force_y, float s) {
  moveto(std::nanf(""), position.x(), position.y(), std::nanf(""), s, force_y, true);
}

void ToolpathExporterFcode::moveto(float feedrate,
                                   float x,
                                   float y,
                                   float z,
                                   float s,
                                   bool force_y,
                                   bool is_travel) {
  if (is_travel || !is_3d_task_ || !std::isnan(z) || is_a_mode_) {
    if (rotary_wait_move_) {
      moveto_(feedrate, x, std::nanf(""), z, s, force_y, is_travel);
      if (disable_rotary_) {
        moveto_(feedrate, std::nanf(""), config_.spinning_axis_coord, std::nanf(""), std::nanf(""), true, true);
        pause(false);
      }
      moveto_(feedrate, std::nanf(""), rotary_y_offset_, std::nanf(""), std::nanf(""), true, true);
      gen_->sync_motion_type2(185, 128, 0.0);
      gen_->sync_motion_type2(179, 128, 2.0);
      moveto_(feedrate, std::nanf(""), y, std::nanf(""), std::nanf(""), force_y, is_travel);
      rotary_wait_move_ = false;
    } else {
      moveto_(feedrate, x, y, z, s, force_y, is_travel);
    }
    return;
  }
  if (std::isnan(x) && std::isnan(y)) {
    return moveto_(feedrate, x, y, z, s, force_y, is_travel);
  }
  // Handle curve engraving steps
  float start_x = curve_x_;
  float start_y = curve_y_;
  float dx = std::isnan(x) ? 0 : x - start_x;
  float dy = std::isnan(y) ? 0 : y - start_y;
  float dist_x = abs(dx);
  float dist_y = abs(dy);
  float gap_x = curve_settings.gap.x();
  float gap_y = curve_settings.gap.y();
  if (2 * dist_x < gap_x && 2 * dist_y < gap_y) {
    return moveto_(feedrate, x, y, z, s, force_y, is_travel);
  }
  int steps = qMax(int(2 * dist_x / gap_x), int(2 * dist_y / gap_y));
  for (int i = 1; i < steps; i++) {
    float ratio = float(i) / steps;
    float step_x = std::isnan(x) ? x : start_x + dx * ratio;
    float step_y = std::isnan(y) ? y : start_y + dy * ratio;
    if (i == 1) {
      moveto_(feedrate, step_x, step_y, z, std::nanf(""), force_y, is_travel);
    } else {
      moveto_(std::nanf(""), step_x, step_y, z, std::nanf(""), force_y, is_travel);
    }
  }
  return moveto_(std::nanf(""), x, y, z, std::nanf(""), force_y, is_travel);
}

void ToolpathExporterFcode::moveto_(float feedrate = std::nanf(""),
                                    float x = std::nanf(""),
                                    float y = std::nanf(""),
                                    float z = std::nanf(""),
                                    float s = std::nanf(""),
                                    bool force_y = false,
                                    bool is_travel = false) {
  int flags = 0;
  float a = std::nanf("");
  if (is_travel) {
    feedrate = travel_speed_;
  }
  if (!isnan(feedrate))
    flags |= FCodeGenerator::move_flag_F;
  if (!isnan(x)) {
    curve_x_ = x;
    flags |= FCodeGenerator::move_flag_X;
  }
  if (!isnan(y)) {
    curve_y_ = y;
    if (is_v2_ && is_a_mode_ && !force_y) {
      if (rotary_y_ratio_ != 1) {
        y = config_.spinning_axis_coord + (y - config_.spinning_axis_coord) * rotary_y_ratio_;
      }
      if (is_travel) {
        gen_->moveto(FCodeGenerator::move_flag_F | FCodeGenerator::move_flag_A,
                     config_.a_travel_speed, std::nanf(""), std::nanf(""),
                     std::nanf(""), y, std::nanf(""));
        y = std::nanf("");
      } else {
        a = y;
        y = std::nanf("");
        flags |= FCodeGenerator::move_flag_A;
      }
    } else {
      flags |= FCodeGenerator::move_flag_Y;
    }
  }
  if (!isnan(z)) {
    flags |= FCodeGenerator::move_flag_Z;
  } else if (is_3d_task_ && !is_a_mode_ && (!isnan(x) || !isnan(y))) {
    // Try to estimate z value for curve engraving
    z = getCurveEngravingHeight();
    if (!isnan(z)) {
      flags |= FCodeGenerator::move_flag_Z;
    }
  }
  if (!isnan(s)) {
    flags |= FCodeGenerator::move_flag_S;
  }
  gen_->moveto(flags, feedrate, x, y, z, a, s);
}

float ToolpathExporterFcode::getCurveEngravingHeight() {
  if (curve_x_ < curve_settings.bbox.left() ||
      curve_x_ > curve_settings.bbox.right() ||
      curve_y_ < curve_settings.bbox.top() ||
      curve_y_ > curve_settings.bbox.bottom()) {
    return std::nanf("");
  }

  cv::Mat query_point = (cv::Mat_<float>(1, 2) << curve_x_, curve_y_);
  int k = 3;
  std::vector<int> indice(k);
  std::vector<float> distances(k);
  cv::flann::SearchParams params(32);
  curve_settings.kdTree->knnSearch(query_point, indice, distances, k, params);
  QVector3D p1 = curve_settings.points[indice[0]];
  float x1 = p1.x(), y1 = p1.y(), z1 = p1.z();
  QVector3D p2 = curve_settings.points[indice[1]];
  float x2 = p2.x(), y2 = p2.y(), z2 = p2.z();
  QVector3D p3 = curve_settings.points[indice[2]];
  float x3 = p3.x(), y3 = p3.y(), z3 = p3.z();

  float denom = ((y2 - y3) * (x1 - x3) + (x3 - x2) * (y1 - y3));
  float z;
  if (denom == 0) {
    float d1 = pow(distances[0], 0.5);
    float d2 = pow(distances[1], 0.5);
    z = (z1 * d2 + z2 * d1) / (d1 + d2);
  } else {
    float u =
        ((y2 - y3) * (curve_x_ - x3) + (x3 - x2) * (curve_y_ - y3)) / denom;
    float v =
        ((y3 - y1) * (curve_x_ - x3) + (x1 - x3) * (curve_y_ - y3)) / denom;
    float w = 1 - u - v;
    z = u * z1 + v * z2 + w * z3;
  }
  return z;
}

QVector<QRect> ToolpathExporterFcode::getBoundingBoxes(QImage* src,
                                                       int merge_offset_x,
                                                       int merge_offset_y,
                                                       int downsample) {
  Q_ASSERT_X(src->format() == QImage::Format_Grayscale8,
             "ToolpathExporterFcode",
             "Input image for getBoundingBoxes() must be Format_Grayscale8");

  QImage src_i = src->copy();
  src_i.invertPixels();
  int w = src_i.width();
  int h = src_i.height();
  cv::Mat img(h, w, CV_8UC1, const_cast<uchar*>(src_i.bits()),
              static_cast<size_t>(src_i.bytesPerLine()));
  cv::Mat emptyImg(h + merge_offset_y * 2, w, CV_8UC1, cv::Scalar(0));
  std::vector<std::vector<cv::Point>> contours;

  if (downsample > 1) {
    cv::Mat downsampledImg;
    cv::resize(img, downsampledImg, cv::Size(w / downsample, h / downsample), 0, 0, cv::INTER_CUBIC);
    cv::findContours(downsampledImg, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
  } else {
    cv::findContours(img, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
  }
  qInfo() << "Initial contour count:" << contours.size();

  for (const auto& c : contours) {
    cv::Rect boundingBox = cv::boundingRect(c);
    int x = boundingBox.x;
    int y = boundingBox.y;
    int w = boundingBox.width;
    int h = boundingBox.height;

    if (downsample > 1) {
      x = std::max(x - 1, 0) * downsample;
      y = std::max(y - 1, 0) * downsample;
      w = (w + 2) * downsample;
      h = (h + 2) * downsample;
    }

    // Add merge offset padding
    // Note: y is already offseted by adding extra height to emptyImg
    cv::Point start(x - merge_offset_x, y);
    cv::Point end(x + w + merge_offset_x - 1, y + h + 2 * merge_offset_y - 1);
    cv::rectangle(emptyImg, start, end, cv::Scalar(255), cv::FILLED);
  }
  cv::findContours(emptyImg, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

  int lastContourCount = contours.size();
  int safeCount = 0;
  // Merge all contours
  while (true) {
    for (const auto& c : contours) {
      cv::rectangle(emptyImg, cv::boundingRect(c), cv::Scalar(255), cv::FILLED);
    }
    cv::findContours(emptyImg, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    if (contours.size() == lastContourCount) {
      break;
    }

    lastContourCount = contours.size();
    safeCount++;
    if (safeCount > 10) {
      break;
    }
  }

  QVector<QRect> res;
  for (const auto& c : contours) {
    cv::Rect boundingBox = cv::boundingRect(c);
    res.append(QRect(boundingBox.x, boundingBox.y, boundingBox.width, boundingBox.height - 2 * merge_offset_y));
  }
  qInfo() << "Final contour count:" << contours.size() << "after" << safeCount << "iterations";

  return res;
}
