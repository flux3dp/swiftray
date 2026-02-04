#include "toolpath-exporter-fcode.h"
#include "toolpath-exporter-constants.h"
#include <constants.h>
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
#include <cmath>
#include <iomanip>
#include <iostream>

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

ToolpathExporterFcode::ToolpathExporterFcode(QTransform move_translate,
                                             int dpi,
                                             const QJsonObject* param,
                                             const QString* thumbnail) noexcept
    : move_translate_(move_translate) {
  qInfo() << "ToolpathExporterFcode init";
  parseParam(param);
  updateMovetoPipeline();
  setDpi(dpi);

  if (is_v2_) {
    if (is_rotary_task_ || with_custom_origin_) {
      magic_number_ = 4;
    } else {
      magic_number_ = 3;
    }
  } else {
    magic_number_ = 1;
  }

  QString type = param->value("type").toString();
  if (type == "gcode") {
    is_gcode_ = true;
    proc.init(-1, nullptr);
  } else {
    proc.init(magic_number_, thumbnail);
  }
  proc.add_metadata("START_WITH_HOME", with_custom_origin_ ? "0" : "1");
  proc.add_metadata("3D_CURVE_TASK", is_3d_task_ ? "1" : "0");
  proc.set_time_est_z_speed(config_.z_speed);
  proc.set_travel_speed(config_.travel_speed);
}

std::string ToolpathExporterFcode::toString() {
  return proc.to_string();
};

void ToolpathExporterFcode::save(QDataStream* out) {
  out->writeRawData(toString().c_str(), proc.total_length());
}

float ToolpathExporterFcode::getTimeCost() {
  return proc.get_time_cost();
}

QJsonObject ToolpathExporterFcode::getMetadata() {
  return proc.get_metadata();
}

void ToolpathExporterFcode::parseParam(const QJsonObject* paramPtr) {
  QJsonObject param = *paramPtr;
  if (param.contains("job_origin")) {
    with_custom_origin_ = true;
    config_.job_origin = QPointF(param["job_origin"].toArray()[0].toDouble(),
                                 param["job_origin"].toArray()[1].toDouble());
  }
  float spinning_axis_coord = param["spin"].toDouble(-1);
  if (spinning_axis_coord >= 0) {
    is_rotary_task_ = true;
    config_.spinning_axis_coord = spinning_axis_coord / canvas_mm_ratio - config_.job_origin.y();
    rotary_y_ratio_ = param["rotary_y_ratio"].toDouble(1);
  }

  QJsonObject workarea = param["workarea"].toObject();
  int width = workarea["width"].toInt();
  int height = workarea["height"].toInt();
  QString model = param["model"].toString();
  float default_path_travel_speed = 7500;
  QJsonObject default_path_acc = {};
  if (model == "fbm1") {
    hardware_ = HardwareType::beamo;
    config_.fg_pwm_limit = 1500;
  } else if (model == "fbb1p") {
    hardware_ = HardwareType::BeamboxPro;
    config_.fg_pwm_limit = 1500;
  } else if (model == "fhexa1") {
    hardware_ = HardwareType::HEXA;
    config_.support_rel_z_move = true;
  } else if (model == "ado1") {
    hardware_ = HardwareType::Ador;
    is_v2_ = true;
    config_.support_modules = true;
    config_.support_rel_z_move = true;
    config_.support_rotary_z_motion = true;
    default_path_travel_speed = 3600;
    default_path_acc["x"] = 500;
    default_path_acc["y"] = 500;
    if (param.contains("prespray")) {
      QJsonArray prespray_arr = param["prespray"].toArray();
      config_.prespray =
          QRectF(prespray_arr[0].toDouble(), prespray_arr[1].toDouble(),
                 prespray_arr[2].toDouble(), prespray_arr[3].toDouble());
    }
  } else if (model == "fbb2") {
    hardware_ = HardwareType::BB2;
    is_v2_ = true;
    config_.z_speed = 5.16;
    config_.support_rel_z_move = true;
    config_.z_premove_speed = 140;
    config_.z_premove_x = 0.0127;
    config_.z_premove_y = 0.0064;
    config_.z_premove_z = 0.0005;
    default_path_acc["x"] = 1000;
    default_path_acc["y"] = 1000;
  } else if (model.startsWith("fhx2rf")) {
    hardware_ = HardwareType::RF;
    is_v2_ = true;
  } else {
    // default beambox
    hardware_ = HardwareType::Beambox;
    config_.fg_pwm_limit = 1500;
  }
  work_area_mm_ = QSizeF(width, height);
  config_.dpmm_preview = 500.0 / width;

  if (config_.support_modules) {
    QJsonObject offset_dict = param["mof"].toObject();
    for (QString module_key : offset_dict.keys()) {
      QJsonArray offset = offset_dict[module_key].toArray();
      config_.module_offsets[module_key.toInt()] =
          QPointF(offset[0].toDouble(), offset[1].toDouble());
    }
  }

  if (param.contains("diode")) {
    config_.enable_diode = true;
    QJsonArray diode_offset = param["diode"].toArray();
    config_.diode_offset =
        QPointF(diode_offset[0].toDouble(), diode_offset[1].toDouble());
  }
  config_.enable_autofocus = param["af"].toBool();
  config_.enable_custom_backlash = param["cbl"].toBool();
  config_.enable_fast_gradient = param["fg"].toBool();
  config_.enable_mock_fast_gradient = param["mfg"].toBool();
  config_.enable_pwm = !param["no_pwm"].toBool();
  config_.enable_multipass_compensation = param["mpc"].toBool();
  config_.enable_segmentation = param["segment"].toBool(true);
  config_.enable_rotary_z_move = param["rotary_z_motion"].toBool() && config_.support_rotary_z_motion;
  config_.is_one_way_printing = param["owp"].toBool();
  config_.is_diode_one_way_engraving = param["diode_owe"].toBool();
  config_.is_reverse_engraving = param["rev"].toBool();
  config_.min_speed = param["min_speed"].toDouble(3);
  config_.travel_speed = param["ts"].toDouble(7500);
  config_.a_travel_speed = param["ats"].toDouble(2000);
  config_.path_travel_speed = param["pts"].toDouble(default_path_travel_speed);
  config_.vector_speed_constraint = param["vsl"].toDouble(0);
  config_.curve_speed_constraint = param["csl"].toDouble(0) / 60;
  config_.padding_acc = param["acc"].toDouble(4000);
  config_.min_engraving_padding = param["mep"].toDouble(NAN);
  config_.min_printing_padding = param["mpp"].toDouble(NAN);
  config_.z_offset = param["z_offset"].toDouble(0);
  config_.loop_compensation = param["loop_compensation"].toDouble() / canvas_mm_ratio;
  config_.printing_top_padding = param["ptp"].toInt(10);
  config_.printing_bot_padding = param["pbp"].toInt(10);
  if (param.contains("nv")) {
    nozzle_settings.voltage = param["nv"].toDouble();
  }
  if (param.contains("npw")) {
    nozzle_settings.pulse_width = param["npw"].toDouble();
  }
  if (param.contains("acc_override")) {
    config_.fill_acc = param["acc_override"].toObject()["fill"].toObject();
    config_.path_acc = param["acc_override"].toObject()["path"].toObject();
  } else {
    config_.path_acc = default_path_acc;
  }

  QJsonArray clip = param["mask"].toArray();
  if (clip.size() == 4) {
    for (int i = 0; i < 4; i++) {
      config_.workarea_clip[i] = clip[i].toDouble();
    }
  }

  QJsonObject curve_obj = param["curve_engraving"].toObject();
  // ToolpathProcessor::set_curve_engraving_data
}

void ToolpathExporterFcode::setTransform(QTransform transform) {
  global_transform_ =
      transform * move_translate_ *
      (is_printing_layer_ ? transform_printing_ : transform_laser_);
}

bool ToolpathExporterFcode::convertStack(const QList<LayerPtr>& layers,
                                         QProgressDialog* dialog) {
  // Step 1. Initialize
  QElapsedTimer t;
  t.start();
  Q_ASSERT_X(!layers.empty(), "ToolpathExporterFcode", "Must input at least one layer");
  total_layer_cnt_ = layers.size();

  progress_ = 0;
  dialog_ = dialog;
  if (dialog != nullptr) {
    connect(dialog, &QProgressDialog::canceled, this, &ToolpathExporterFcode::handleCancel);
  }

  // Step 2. Handle pre-task
  if (is_v2_) {
    proc.start_task_script_block("xMIN", "0003");
    proc.miscellaneous_cmd(1);
    if (is_rotary_task_ && config_.enable_rotary_z_move) {
      moveZ(-1);
    }
    if (magic_number_ >= 4 && !with_custom_origin_) {
      proc.grbl_system_cmd(0);
    }
  } else {
    proc.home();
  }
  proc.set_toolhead_pwm(0);
  if (is_v2_) {
    travel(0, 0);
  }
  if (is_3d_task_ && !isnan(curve_settings.safe_height)) {
    if (is_v2_) {
      proc.sync_motion_type2(179, 5.0);
    }
    if (config_.z_premove_speed){
      moveto(config_.z_premove_speed, NAN, NAN, curve_settings.safe_height);
    } else {
      moveZ(curve_settings.safe_height);
    }
  }

  // Supporting for spinning axis
  if (is_rotary_task_) {
    if (is_v2_) {
      if (!config_.enable_rotary_z_move) {
        travel(0, config_.spinning_axis_coord + 1, true);
        travel(0, config_.spinning_axis_coord - 1, true);
        travel(0, config_.spinning_axis_coord, true);
        pause(false);
      }
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
    proc.miscellaneous_cmd(0);
  }

  proc.set_time_est_acc(config_.padding_acc);

  // End of pre-task script
  if (is_v2_) {
    proc.end_task_script_block();
  }
  if (cancelled_) {
    return false;
  }

  // Step 3. Init bitmap canvas
  laser_bitmap_ = QImage(std::ceil(work_area_mm_.width() * config_.dpmm_x),
                         std::ceil(work_area_mm_.height() * config_.dpmm_y),
                         QImage::Format_Grayscale8);
  laser_bitmap_.fill(Qt::white);
  printing_bitmap_ =
      QImage(std::ceil(work_area_mm_.width() * config_.dpmm_printing),
             std::ceil(work_area_mm_.height() * config_.dpmm_printing),
             QImage::Format_Grayscale8);
  printing_bitmap_.fill(Qt::white);
  // Limit preview width to 500
  preview_bitmap_ =
      QImage(std::round(work_area_mm_.width() * config_.dpmm_preview),
             std::ceil(work_area_mm_.height() * config_.dpmm_preview),
             QImage::Format_ARGB32);
  qInfo() << "[Canvas size]";
  qInfo() << "laser_bitmap_" << laser_bitmap_.size();
  qInfo() << "printing_bitmap_" << printing_bitmap_.size();
  qInfo() << "preview_bitmap_" << preview_bitmap_.size();

  // Step 4. Handle each layer
  int visible_layer_cnt = 1; // v2 task info
  int last_module;
  QString last_color;
  QString last_sub_type;
  QJsonArray post_config;
  for (auto layer_rit = layers.crbegin(); layer_rit != layers.crend();
       layer_rit++) {
    qInfo() << "[Export] Output layer: " << (*layer_rit)->name();
    total_repeat_times_ = (*layer_rit)->repeat();
    processed_repeat_times_ = 0;
    if ((*layer_rit)->isVisible() && total_repeat_times_ > 0) {
      if (cancelled_) {
        return false;
      }
      onProgressChanged(0, true);

      current_layer_ = *layer_rit;
      updateLayerParam();

      float layer_height = current_layer_->targetHeight();
      float layer_z_step = current_layer_->stepHeight();
      if (is_v2_) {
        proc.write_string("TASK", 4);
        // Write transition script
        proc.start_task_script_block("TRAN", nullptr);
        if (!is_gcode_ && config_.support_modules) {
          if (is_rotary_task_ && config_.enable_rotary_z_move) {
            moveZ(1);
          }
          QPointF tran_pos;
          if (with_custom_origin_) {
            tran_pos = QPointF(0, 0);
          } else if (hardware_ == ToolpathExporterFcode::HardwareType::Ador) {
            tran_pos = QPointF(215, 150);
          } else {
            tran_pos = QPointF(work_area_mm_.width() / 2, work_area_mm_.height() / 2);
          }
          if (is_rotary_task_ && config_.enable_rotary_z_move) {
            travel(NAN, 0, true);
            travel(tran_pos.x(), NAN, true);
            travel(NAN, tran_pos.y(), true);
            proc.sync_motion_type2(179, 3.0);
          } else {
            travel(tran_pos, true);
          }
          proc.sync_grbl_motion(0);
          proc.flux_custom_cmd(168);
          proc.flux_custom_cmd(174);
          proc.user_selection_cmd(0);
          if (!with_custom_origin_) {
            proc.grbl_system_cmd(0);
          }
          proc.sync_grbl_motion(0);
          proc.miscellaneous_cmd(0);
        }
        proc.end_task_script_block();
        // Write main script
        proc.start_task_script_block("MAIN", nullptr);
        if (is_rotary_task_) {
          if (config_.enable_rotary_z_move) {
            rotary_wait_move_ = true;
            rotary_y_offset_ = config_.spinning_axis_coord - module_offset_.y();
          } else {
            travel(NAN, config_.spinning_axis_coord - module_offset_.y(), true);
          }
          module_offset_.setY(0);
        } else {
          proc.sync_motion_type2(179, 2.0);
        }
        proc.sync_grbl_motion(0);
        proc.miscellaneous_cmd(1);
      }
      if (config_.enable_diode) {
        proc.set_toolhead_laser_module(current_layer_->isUseDiode());
      }
      if (with_custom_origin_) {
        module_offset_ += config_.job_origin;
      }
      if (has_focus_adjust_ && focus_adjust_ > 0) {
        proc.sync_motion_type2(184, focus_adjust_);
      } else if (config_.enable_autofocus && !did_home_z_ && layer_height > 0) {
        moveZ(-1);
        did_home_z_ = true;
      }
      proc.set_toolhead_pwm(-current_layer_->power() / 100);

      is_handling_main_work_ = true;
      if (is_printing_layer_) {
        if (!with_print_task_) {
          with_print_task_ = true;
        }
        convertPrintingLayer();
        proc.set_toolhead_pwm(0);
        moveto(NAN, NAN, NAN, NAN, NAN, 0);
      } else {
        for (processed_repeat_times_ = 0; processed_repeat_times_ < total_repeat_times_; processed_repeat_times_++) {
          if (has_focus_adjust_ && focus_step_ > 0 && processed_repeat_times_ > 0) {
            proc.sync_motion_type2(184, focus_step_);
          } else if (config_.enable_autofocus && layer_height > 0) {
            double target_z = 17.0 - layer_height - config_.z_offset + processed_repeat_times_ * layer_z_step;
            target_z = round(qMax(qMin(target_z, 17.0), 0.0) * 100) / 100;
            moveZ(target_z);
          }
          convertLaserLayer();
          proc.set_toolhead_pwm(0);
        }
        moveto(NAN, NAN, NAN, NAN, NAN, 0);
        if (has_focus_adjust_ && focus_step_ > 0 && total_repeat_times_ > 1) {
          float total_step = focus_step_ * (total_repeat_times_ - 1);
          proc.sync_motion_type2(184, -total_step);
        }
      }
      is_handling_main_work_ = false;
      if (has_focus_adjust_ && focus_adjust_ > 0) {
        proc.sync_motion_type2(184, -focus_adjust_);
      }
      if (is_v2_) {
        proc.end_task_script_block();
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
        proc.write_task_info(task_info);
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
    if (cancelled_) {
      break;
    }
    total_repeat_times_ = processed_repeat_times_ = 1;
    onProgressChanged(0, true);
    processed_layer_cnt_++;
  }
  if (cancelled_) {
    return false;
  }

  // Step 5. Handle printing test, prespray task if needed
  if (!is_gcode_ && is_v2_ && with_print_task_ && !config_.prespray.isEmpty()) {
    // deprecated
  }
  if (cancelled_) {
    return false;
  }

  // Step 6. Handle post-task
  if (is_v2_) {
    proc.start_task_script_block("xMIN", "0004");
  }
  if (is_3d_task_ && !isnan(curve_settings.safe_height)) {
    if (config_.z_premove_speed){
      moveto(config_.z_premove_speed, NAN, NAN, curve_settings.safe_height);
    } else {
      moveZ(curve_settings.safe_height);
    }
  }
  if (is_rotary_task_) {
    if (is_v2_) {
      if (config_.enable_rotary_z_move) {
        moveZ(1);
      }
      travel(NAN, config_.spinning_axis_coord);
      travel(NAN, 0, true);
      proc.sync_grbl_motion(36);
      is_a_mode_ = false;
      travel(0, 0);
    } else {
      travel(0, config_.spinning_axis_coord);
    }
    if (rotary_y_ratio_ != 1) {
      rotary_y_ratio_ = 1;
    }
  } else {
    travel(0, 0);
  }
  proc.write_boundary_to_metadata();
  if (is_v2_) {
    if (is_rotary_task_ && config_.enable_rotary_z_move) {
      proc.sync_motion_type2(185, 0.0);
    } else {
      proc.sync_motion_type2(179, 3.0);
    }
    proc.end_task_script_block();
    proc.end_content();
    proc.write_post_config(post_config);
  }
  proc.terminated();
  qInfo() << "[Export] Took " << t.elapsed() << " milliseconds";
  return true;
}

void ToolpathExporterFcode::updateLayerParam() {
  // deprecated
}

void ToolpathExporterFcode::updateOffset() {
  if (config_.support_modules) {
    module_offset_ = config_.module_offsets[layer_module_];
  } else if (config_.enable_diode && current_layer_->isUseDiode()) {
    module_offset_ = config_.diode_offset;
  } else {
    module_offset_ = QPointF(0, 0);
  }
}

void ToolpathExporterFcode::updateClip() {
  // deprecated
}

void ToolpathExporterFcode::convertLaserLayer() {
  setTransform();
  polygons_mutex_.lock();
  layer_polygons_.clear();
  layer_bitmaps_.clear();
  polygons_mutex_.unlock();
  is_handling_bitmap_ = false;
  layer_painter_ = std::make_unique<QPainter>(&laser_bitmap_);
  layer_painter_->setClipRect(clip_area_);
  preview_painter_ = std::make_unique<QPainter>(&preview_bitmap_);
  preview_bitmap_.fill(Qt::transparent);
  bitmap_dirty_area_ = QRectF();
  element_cnt_[0] = 0, element_cnt_[1] = 0;
  // First pass: Generate path list and bitmap list and draw filled path by layer_painter_
  for (auto& shape : current_layer_->children()) {
    convertShape(shape);
  }
  path_utils_.sortAndPreprocessPolygons(layer_polygons_);
  if (this->cancelled_) return;
  onProgressChanged(0.05, true);
  total_element_cnt_ = element_cnt_[0] + element_cnt_[1] + layer_bitmaps_.size();

  // Part 1: Generate path fcode
  outputLayerPathFcode();
  if (this->cancelled_) return;
  onProgressChanged(0.05 + 0.95 * element_cnt_[0] / total_element_cnt_, true);

  // Part 2: Generate filled path fcode
  outputBitmapFcode();
  if (this->cancelled_) return;
  onProgressChanged(0.05 + 0.95 * (element_cnt_[0] + element_cnt_[1]) / total_element_cnt_, true);

  // Second pass
  // Part 3: Generate bitmap fcode
  is_handling_bitmap_ = true;
  for (auto& shape : layer_bitmaps_) {
    // Note: Bitmap list is already reversed for first-depth shapes
    // When converting group, reverse order of children and ignore paths
    convertShape(shape);
  }
  layer_painter_->end();
  preview_painter_->end();
}

void ToolpathExporterFcode::convertPrintingLayer() {
  // Overwrite repeat, count progress as a whole
  processed_repeat_times_ = 0, total_repeat_times_ = 1;

  // deprecated
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
    if (is_v2_ && !is_gcode_) {
      preview_painter_->save();
      preview_painter_->setTransform(getPreviewTransform(), false);
      preview_painter_->drawImage(new_dirty_area.topLeft(), transformed_image);
      preview_painter_->restore();
    }
    if (bmp->pwm() || is_printing_layer_) {
      if (!config_.enable_fast_gradient) {
        transformed_image = transformed_image.convertToFormat(QImage::Format_Mono).convertToFormat(QImage::Format_Grayscale8);
      }
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
    if (is_v2_ && !is_gcode_) {
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
    if (is_v2_ && !is_gcode_) {
      preview_painter_->setPen(Qt::NoPen);
      preview_painter_->setBrush(Qt::black);
      preview_painter_->drawPath(transformed_path * getPreviewTransform());
      preview_painter_->setBrush(Qt::NoBrush);
    }
    bitmap_dirty_area_ = bitmap_dirty_area_.united(path_bounding_rect);
    element_cnt_[1]++;
  } else if (is_printing_layer_) {
    // Note: This is for dev convinience
    // Path in BVG input should already been converted to image
    layer_painter_->drawPath(transformed_path);
    if (is_v2_ && !is_gcode_) {
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
    if (is_v2_ && !is_gcode_) {
      preview_painter_->drawPath(transformed_path * getPreviewTransform());
    }
    element_cnt_[0]++;
    polygons_mutex_.unlock();
  }
}

void ToolpathExporterFcode::outputLayerPathFcode() {
  bool should_set_acc = !config_.path_acc.isEmpty();
  polygons_mutex_.lock();
  for (auto& poly : layer_polygons_) {
    if (poly.empty()) {
      continue;
    }
    if (should_set_acc) {
      setAcceleration(
        config_.path_acc["x"].toDouble(NAN),
        config_.path_acc["y"].toDouble(NAN),
        config_.path_acc["z"].toDouble(NAN),
        config_.path_acc["a"].toDouble(NAN)
      );
    }
    setTravelSpeed(config_.path_travel_speed);
    handlePathWalk(poly.first(), false);
    for (QPointF& point : poly) {
      handlePathWalk(point, true);
    }
    handlePathWalk(poly.last(), false);
    setTravelSpeed(config_.travel_speed);
    // Reset path_acc
    if (should_set_acc) {
      proc.sync_grbl_motion(151);
      proc.set_time_est_acc(config_.padding_acc);
    }
  }
  polygons_mutex_.unlock();
}

void ToolpathExporterFcode::handlePathWalk(QPointF point, bool should_emit) {
  // deprecated
}

// Handling bitmap and filled path
void ToolpathExporterFcode::outputBitmapFcode(bool pwm_engraving) {
  if (bitmap_dirty_area_.width() == 0) {
    qInfo() << "Skip: empty bitmap";
  } else {
    bool should_set_acc = !config_.fill_acc.isEmpty();
    if (should_set_acc) {
      setAcceleration(
        config_.fill_acc["x"].toDouble(NAN),
        config_.fill_acc["y"].toDouble(NAN),
        config_.fill_acc["z"].toDouble(NAN),
        config_.fill_acc["a"].toDouble(NAN)
      );
    }
    QVector<QRect> bboxes = getBoundingBoxes(&laser_bitmap_, padding_px_, 5, dpmm_y() / 5);
    char gradient_print_mode = 0;
    if (config_.enable_fast_gradient) {
      gradient_print_mode = pwm_engraving ? config_.print_modes[0] : config_.print_modes[1];
    }
    for (auto bbox : bboxes) {
      if (gradient_print_mode != 0) {
        proc.turn_on_gradient_print_mode(gradient_print_mode);
      }
      QPointF start = getPointInMM(bbox.topLeft());
      travel(start);
      int step = bbox.width();
      if (pwm_engraving && config_.fg_pwm_limit) {
        step = qMax(config_.fg_pwm_limit - padding_px_ * 2, 100);
      }
      for (int left = bbox.left(); left <= bbox.right(); left += step) {
        bitmap_progress_unit_ = 0.95 * element_cnt_[1] / total_element_cnt_ / bboxes.size() / qCeil(bbox.width() / step) / bbox.height();
        int right = qMin(left + step, bbox.left() + bbox.width()) - 1;
        QRect sliced_box = QRect(bbox);
        sliced_box.setLeft(left);
        sliced_box.setRight(right);
        rasterBitmap(laser_bitmap_, sliced_box, pwm_engraving);
      }
      if (config_.enable_fast_gradient) {
        proc.turn_off_gradient_print_mode();
      }
    }
    if (should_set_acc) {
      // Reset fill_acc
      proc.sync_grbl_motion(151);
      proc.set_time_est_acc(config_.padding_acc);
    }
  }

  // Clear canvas
  layer_painter_->fillRect(bitmap_dirty_area_, Qt::white);
  bitmap_dirty_area_ = QRectF();
}

bool ToolpathExporterFcode::rasterBitmap(const QImage& layer_image,
                                         QRect bbox,
                                         bool pwm_engraving) {
  // deprecated
  return true;
}

bool ToolpathExporterFcode::rasterLine(const uchar* data_ptr,
                                       int left_bound,   // included
                                       int right_bound,  // included
                                       int y,
                                       bool reverse_raster_dir) {
  // LaserBitmapFactory::iterate_x
}

bool ToolpathExporterFcode::rasterLineHighSpeed(const uchar* data_ptr,
                                                int left_bound,   // included
                                                int right_bound,  // included
                                                int y,
                                                bool reverse_raster_dir) {
  // LaserBitmapFactory::fg_iterate_x
}

bool ToolpathExporterFcode::rasterLineHighSpeedPwm(const uchar* data_ptr,
                                                   int left_bound,   // included
                                                   int right_bound,  // included
                                                   int y,
                                                   bool reverse_raster_dir) {
  // LaserBitmapFactory::fg_iterate_x_pwm
}

void ToolpathExporterFcode::outputLayerPrintingFcode(float halftone_multiplier) {
  // deprecated
}

QByteArray ToolpathExporterFcode::generateNozzleSettingPayload(
    int saturation,
    bool use_default) {
  QByteArray payload;
  // toolpath-utils generate_nozzle_setting_payload
  return payload;
}

void ToolpathExporterFcode::sliceBox(QList<QList<QList<int>>>* sliced_boxes,
                                     QRect box,
                                     int multipass) {
  // deprecated
}

std::tuple<QRect, QRect> ToolpathExporterFcode::getPresprayBbox() {
  // PrinterBitmapFactory::set_preparatory_task_bbox
}

// Note: -printing_slice_height < offset_y < printing_slice_height
void ToolpathExporterFcode::writeSimpleFilledTaskCode(QRect bbox,
                                                      int offset_y) {
  // deprecated
}

void ToolpathExporterFcode::writePreviewImage() {
  if (is_gcode_) return;
  QRect dirty_area = getPreviewTransform().mapRect(bitmap_dirty_area_).toAlignedRect();
  clearWhite(&preview_bitmap_, dirty_area);
  QByteArray byteArray;
  QBuffer buffer(&byteArray);
  preview_bitmap_.save(&buffer, "PNG");
  proc.write_string("PREV", 4);
  proc.write_string(byteArray.data(), byteArray.size(), true);
}

void ToolpathExporterFcode::clearWhite(QImage* src, QRect dirty_area) {
  Q_ASSERT_X(src->allGray(), "ToolpathExporterFcode",
             "Input image for clearWhite() must be grayscaled");
  Q_ASSERT_X(src->format() == QImage::Format_ARGB32, "ToolpathExporterFcode",
             "Input image for clearWhite() must be Format_ARGB32");
  int left = qMax(dirty_area.x(), 0);
  int right = qMin(dirty_area.x() + dirty_area.width(), src->width());
  int top = qMax(dirty_area.y(), 0);
  int bottom = qMin(dirty_area.y() + dirty_area.height(), src->height());
  for (int y = top; y < bottom; ++y) {
    QRgb* ptr = (QRgb*)src->scanLine(y);
    for (int x = left; x < right; ++x) {
      int gray = qGray(ptr[x]);
      if (gray == WHITE_PIXEL) {
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
  proc.pause(to_standby_position);
  if (!to_standby_position) {
    disable_rotary_ = to_standby_position;
  }
}

// ======== start of move functions ========
void ToolpathExporterFcode::updateMovetoPipeline() {
  // ToolpathProcessor::update_moveto_pipeline
}

void ToolpathExporterFcode::moveZ(float z) {
  moveto(NAN, NAN, NAN, z);
}

void ToolpathExporterFcode::travel(float x, float y, bool force_y, float s) {
  moveto(NAN, x, y, NAN, NAN, s, force_y, true);
}

void ToolpathExporterFcode::travel(QPointF position, bool force_y, float s) {
  moveto(NAN, position.x(), position.y(), NAN, NAN, s, force_y, true);
}

void ToolpathExporterFcode::moveto(float feedrate,
                                   float x,
                                   float y,
                                   float z,
                                   float a,
                                   float s,
                                   bool force_y,
                                   bool is_travel) {
  // ToolpathProcessor::moveto
}

void ToolpathExporterFcode::pipelineMoveto(int idx, MoveArgs args) {
  // ToolpathProcessor::pipeline_moveto
}

void ToolpathExporterFcode::rotaryMotionGenerator(
    MoveArgs args,
    std::function<void(MoveArgs args)> callback) {
  // ToolpathProcessor::rotary_motion_generator
}

void ToolpathExporterFcode::curveEngravingMotionGenerator(
    MoveArgs args,
    std::function<void(MoveArgs args)> callback) {
  // ToolpathProcessor::curve_engraving_motion_generator
}

void ToolpathExporterFcode::zPremoveMotionGenerator(
    MoveArgs args,
    std::function<void(MoveArgs args)> callback) {
  // ToolpathProcessor::z_premove_motion_generator
}

void ToolpathExporterFcode::_moveto(MoveArgs args) {
  // ToolpathProcessor::_moveto
}
// ======== end of move functions ========

QVector<QRect> ToolpathExporterFcode::getBoundingBoxes(QImage* src,
                                                       int merge_offset_x,
                                                       int merge_offset_y,
                                                       int downsample) {
  // factory-utils get_bounding_boxes
}

void ToolpathExporterFcode::handleCancel() {
  this->cancelled_ = true;
}

/**
 * Update progress and emit signal if necessary
 * Also check if the process is cancelled
 */
void ToolpathExporterFcode::onProgressChanged(double value, bool absolute) {
  current_progress_ = absolute ? value : current_progress_ + value;
  // 5% for pre-task, 5% for printing test and post-task
  int new_progress = 5 + 90 * (processed_layer_cnt_ + (processed_repeat_times_ + current_progress_) / total_repeat_times_) / total_layer_cnt_;
  if (new_progress > progress_) {
    progress_ = new_progress;
    if (dialog_ != nullptr) {
      dialog_->setValue(progress_);
    } else {
      Q_EMIT progressChanged(progress_);
    }
  }
  // Always call processEvents to receive the cancellation signal quickly
  QCoreApplication::processEvents();
}
