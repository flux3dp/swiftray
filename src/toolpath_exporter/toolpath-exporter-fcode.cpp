#include "toolpath-exporter-fcode.h"
#include "toolpath_exporter/factories/laser.h"
#include "toolpath_exporter/factories/printer-4c.h"
#include "toolpath_exporter/factories/printer.h"
#include "toolpath_exporter/factories/uv.h"
#include "toolpath_exporter/macros/beamo2.h"
#include "toolpath_exporter/macros/prespray.h"
#include "toolpath_exporter/macros/uv1.h"
#include "windows/image-sharpen-dialog.h"
#include <QBuffer>
#include <QCoreApplication>
#include <QDebug>
#include <QElapsedTimer>

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

ToolpathExporterFcode::ToolpathExporterFcode(
    const QJsonObject* param,
    const QString* thumbnail) noexcept {
  qInfo() << "ToolpathExporterFcode init";
  parseParam(*param);

  if (is_v2_) {
    if (is_rotary_task_ || has_job_origin_) {
      magic_number_ = 4;
    } else {
      magic_number_ = 3;
    }
  } else {
    magic_number_ = 1;
  }

  QString type = param->value("type").toString();
  if (type == "gcode") {
    proc.init(-1, nullptr);
  } else {
    proc.init(magic_number_, thumbnail);
  }
  proc.add_metadata("START_WITH_HOME", has_job_origin_ ? "0" : "1");
  proc.add_metadata("3D_CURVE_TASK", is_3d_task_ ? "1" : "0");
  proc.set_time_est_z_speed(hw_profile.z_speed);
  proc.set_travel_speed(config_.travel_speed);

  if (hardware_ == HardwareType::BM2) {
    macros = std::make_shared<Beamo2Macros>(&proc, config_.travel_speed);
    if (param->contains("machine_limit_position")) {
      QJsonDocument machine_limit_position = QJsonDocument::fromJson(
          param->value("machine_limit_position").toString().toUtf8());
      if (hasattr(macros, MacroFunc::set_ref_position)) {
        macros->set_ref_position(
            machine_limit_position["x_left"].toDouble(NAN),
            machine_limit_position["y_max"].toDouble(NAN),
            machine_limit_position["y_lid"].toDouble(NAN),
            machine_limit_position["z_magnet"].toDouble(NAN),
            machine_limit_position["z_table"].toDouble(NAN),
            machine_limit_position["z_brush"].toDouble(NAN));
      }
    }
  } else if (hardware_ == HardwareType::UV) {
    macros = std::make_shared<UV1Macros>(&proc, config_.travel_speed);
  }
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

void ToolpathExporterFcode::setTransform(QTransform transform) {
  global_transform_ = transform * transform_base_;
}

void ToolpathExporterFcode::parseParam(const QJsonObject& param) {
  if (param.contains("job_origin")) {
    has_job_origin_ = true;
    config_.job_origin = QPointF(param["job_origin"].toArray()[0].toDouble(),
                                 param["job_origin"].toArray()[1].toDouble());
  }
  float spinning_axis_coord = param["spin"].toDouble(-1);
  if (spinning_axis_coord >= 0) {
    is_rotary_task_ = true;
    config_.spinning_axis_coord_mm = spinning_axis_coord / CANVAS_MM_RATIO - config_.job_origin.y();
    config_.rotary_y_ratio = param["rotary_y_ratio"].toDouble(1);
  }

  QString model = param["model"].toString();
  hardware_ = model_to_hardware_type(model);
  hw_profile = HW_PROFILE[hardware_];
  is_v2_ = hw_profile.fcode_version == 2;
  if (SUPPORT_INFO.contains(hardware_)) {
    support_info = SUPPORT_INFO[hardware_];
  }
  QJsonObject workarea = param["workarea"].toObject();
  double width = workarea["width"].toDouble(hw_profile.width);
  double height = workarea["height"].toDouble(hw_profile.length);
  work_area_mm_ = QSizeF(width, height);
  qInfo() << "Canvas size" << work_area_mm_;

  if (!has_job_origin_) {
    config_.home_pos = hw_profile.home_position;
  }

  if (param.contains("prespray")) {
    QJsonArray prespray_arr = param["prespray"].toArray();
    config_.prespray =
        QRectF(prespray_arr[0].toDouble(), prespray_arr[1].toDouble(),
               prespray_arr[2].toDouble(), prespray_arr[3].toDouble());
  }

  if (support_info.MODULES) {
    QJsonObject offset_dict = param["mof"].toObject();
    for (QString module_key : offset_dict.keys()) {
      QJsonArray offset = offset_dict[module_key].toArray();
      config_.module_offsets[LayerModule(module_key.toInt())] =
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
  config_.enable_rotary_z_move = param["rotary_z_motion"].toBool() && support_info.ROTARY_Z_MOTION;
  config_.is_one_way_printing = param["owp"].toBool();
  config_.is_diode_one_way_engraving = param["diode_owe"].toBool();
  config_.is_reverse_engraving = param["rev"].toBool();
  config_.skip_prespray = param["skip_prespray"].toBool();
  config_.min_speed = param["min_speed"].toDouble(3);
  config_.travel_speed = param["ts"].toDouble(7500);
  config_.a_travel_speed = param["ats"].toDouble(2000);
  config_.path_travel_speed = param["pts"].toDouble(7500);
  config_.vector_speed_limit = param["vsl"].toDouble(0);
  config_.curve_speed_limit = param["csl"].toDouble(0);
  config_.padding_acc = param["acc"].toDouble(4000);
  config_.min_engraving_padding = param["mep"].toDouble(NAN);
  config_.min_printing_padding = param["mpp"].toDouble(NAN);
  config_.z_offset = param["z_offset"].toDouble(0);
  config_.loop_compensation = param["loop_compensation"].toDouble() / CANVAS_MM_RATIO;
  config_.engraving_erode = param["engraving_erode"].toDouble(0);
  config_.printing_top_padding = param["ptp"].toInt(-1);
  config_.printing_bot_padding = param["pbp"].toInt(-1);
  config_.printing_slice_width = param["psw"].toInt(-1);
  config_.printing_slice_height = param["psh"].toInt(-1);
  config_.watt = param["watt"].toInt(0);
  config_.nozzle_voltage = param["nv"].toDouble(NAN);
  config_.nozzle_pulse_width = param["npw"].toDouble(NAN);
  config_.expected_module = MachineModules(param["expected_module"].toInt(0));
  config_.use_ga_reorder = param["use_ga_reorder"].toBool(true);
  config_.enable_s_curve = param["s_curve"].toBool(false);

  if (param.contains("acc_override")) {
    QJsonObject acc_obj = param["acc_override"].toObject();
    if (acc_obj.contains("fill")) {
      config_.fill_acc.updateFromJson(acc_obj["fill"].toObject());
    }
    if (acc_obj.contains("path")) {
      config_.path_acc.updateFromJson(acc_obj["path"].toObject());
    }
  } else if (PATH_ACCELERATION_DATA.contains(hardware_)) {
    config_.path_acc = PATH_ACCELERATION_DATA[hardware_];
  }

  QJsonArray clip = param["mask"].toArray();
  if (clip.size() == 4) {
    config_.workarea_clip.top = clip[0].toDouble();
    config_.workarea_clip.right = clip[1].toDouble();
    config_.workarea_clip.bottom = clip[2].toDouble();
    config_.workarea_clip.left = clip[3].toDouble();
  }

  is_3d_task_ = proc.set_curve_engraving_data(
      param["curve_engraving"].toObject(), config_.job_origin,
      QSizeF(hw_profile.width, hw_profile.length), config_.workarea_clip);
  if (is_3d_task_) {
    if (Z_PREMOVE_DATA.contains(hardware_)) {
      proc.set_z_premove(Z_PREMOVE_DATA[hardware_]);
    }
    config_.z_acc = param["curve_engraving"].toObject()["acceleration"].toDouble(NAN);
  }
}

InwardRect ToolpathExporterFcode::getClipRect(InwardRect current,
                                              QPointF offset,
                                              LayerModule module,
                                              bool rotary) {
  InwardRect res = {current.top, current.right, current.bottom, current.left};
  if (support_info.MODULES) {
    InwardRect module_clip = get_boundary(hardware_, module);
    module_clip.right = qMax(module_clip.right - offset.x(), 0.0);
    module_clip.left = qMax(module_clip.left + offset.x(), 0.0);
    if (rotary) {
      module_clip.top = 0;
      module_clip.bottom = 0;
    } else {
      module_clip.top = qMax(module_clip.top + offset.y(), 0.0);
      module_clip.bottom = qMax(module_clip.bottom - offset.y(), 0.0);
    }
    res.top = qMax(res.top, module_clip.top);
    res.right = qMax(res.right, module_clip.right);
    res.bottom = qMax(res.bottom, module_clip.bottom);
    res.left = qMax(res.left, module_clip.left);
  }
  return res;
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

  for (auto& layer : layers) {
    LayerModule layer_module = LayerModule(layer->module());
    all_modules_.insert(layer_module);
    if (!has_printing_task_)
      has_printing_task_ = is_printing_module(layer_module);
  }

  // Step 2. Handle pre-task
  if (is_v2_) {
    proc.start_task_script_block("xMIN", "0003");
    if (support_info.PRINTING_SCRIPTS) {
      if ((support_info.MODULES && has_printing_task_) ||
          (is_rotary_task_ && config_.enable_rotary_z_move)) {
        // recording the z position
        proc.grbl_system_cmd(1);  // $HZ
        proc.sync_grbl_motion(0);
        if (!(is_rotary_task_ && config_.enable_rotary_z_move)) {
          proc.sync_motion_type2(185, 0.0);  // move back to z position
          proc.sync_grbl_motion(0);
        }
      }
    } else if (is_rotary_task_ && config_.enable_rotary_z_move) {
      homeZAxis();
    }
    proc.miscellaneous_cmd(1);
    if (magic_number_ >= 4 && !has_job_origin_) {
      proc.grbl_system_cmd(0);
    }
  } else {
    proc.home();
  }
  proc.set_toolhead_pwm(0);
  if (is_v2_) {
    backToHome();
  }
  if (is_3d_task_) {
    if (is_v2_) {
      // M137P179q5 to make sure z-axis homed
      proc.sync_motion_type2(179, 5.0);
    }
    if (!isnan(proc.curve_engraving_data->safe_height)) {
      NamedArgs args = NamedArgs().rz(proc.curve_engraving_data->safe_height);
      if (proc.z_premove_.speed) {
        args.f = proc.z_premove_.speed;
      }
      proc.moveto(args);
    }
  }

  // Supporting for spinning axis
  if (is_rotary_task_) {
    proc.set_travel_speed(config_.a_travel_speed, true);
    if (is_v2_) {
      if (!config_.enable_rotary_z_move) {
        proc.moveto(NamedArgs().rx(0).ry(config_.spinning_axis_coord_mm + 1).set_is_travel().set_force_y());
        proc.moveto(NamedArgs().rx(0).ry(config_.spinning_axis_coord_mm - 1).set_is_travel().set_force_y());
        proc.moveto(NamedArgs().rx(0).ry(config_.spinning_axis_coord_mm).set_is_travel().set_force_y());
        proc.pause(false);
      }
      proc.set_a_mode(true);
    } else {
      proc.moveto(NamedArgs().rx(0).ry(config_.spinning_axis_coord_mm + 1).set_is_travel());
      proc.moveto(NamedArgs().rx(0).ry(config_.spinning_axis_coord_mm - 1).set_is_travel());
      proc.moveto(NamedArgs().rx(0).ry(config_.spinning_axis_coord_mm).set_is_travel());
      // Set rotary mode in beambox-firmware
      proc.pause(false);
    }
    proc.set_rotary_axis(config_.spinning_axis_coord_mm);
    if (config_.rotary_y_ratio != 1) {
      proc.set_rotary_y_ratio(config_.rotary_y_ratio);
    }
  }

  // Enable GCode Boost in beambox-firmware
  proc.pause(true);

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

  // Step 3. check_intersection
  if (!has_job_origin_) {
    if (hasattr(macros, MacroFunc::set_should_retract_table)) {
      QSet<CollisionRegions> res = check_intersection(
          layers, hardware_, all_modules_, config_.padding_acc,
          config_.module_offsets, config_.min_printing_padding,
          config_.min_engraving_padding);
      if (!res.contains(CollisionRegions::FBM2_SLIDING_TABLE)) {
        macros->set_should_retract_table(false);
      }
    }
  }

  // Step 4. Handle each layer in reverse order
  for (auto layer = layers.crbegin(); layer != layers.crend(); layer++) {
    current_layer_ = *layer;
    layer++;
    if (layer != layers.crend() &&
        current_layer_->type() == Layer::Type::Line &&
        (*layer)->type() != Layer::Type::Line &&
        current_layer_->name() + "-filled" == (*layer)->name()) {
      // Swiftray create two layers for line + filled path, handle them together
      current_layer_2_ = *layer;
      qInfo() << "[Export] Handle layers together" << current_layer_->name() << current_layer_2_->name();
    } else {
      layer--;
      current_layer_2_ = nullptr;
    }
    convertLayer();
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
  outputPrintingTestFcode();
  if (cancelled_) {
    return false;
  }

  // Step 6. Write additional metadata
  if (support_info.MODULE_CHECK_METADATA) {
    bool has_4c = all_modules_.contains(LayerModule::PRINTER_4C);
    bool has_1064 = all_modules_.contains(LayerModule::LASER_1064);
    MachineModules required_module = MachineModules::NONE;
    if (has_4c && has_1064) {
      required_module = MachineModules::PRINTER_4C_WITH_1064;
    } else if (has_4c) {
      required_module = MachineModules::PRINTER_4C;
    } else if (has_1064) {
      required_module = MachineModules::LASER_1064;
    }
    proc.add_metadata("REQUIRED_HEADTYPE", int(required_module));

    if (config_.expected_module != MachineModules::NONE) {
      MachineModules forbidden_headtype = MachineModules::NONE;
      if (config_.expected_module == MachineModules::NONE) {
        forbidden_headtype = MachineModules::PRINTER_4C_WITH_1064;
      } else if (config_.expected_module == MachineModules::PRINTER_4C) {
        forbidden_headtype = MachineModules::LASER_1064;
      } else if (config_.expected_module == MachineModules::LASER_1064) {
        forbidden_headtype = MachineModules::PRINTER_4C;
      }
      proc.add_metadata("FORBIDDEN_HEADTYPE", int(forbidden_headtype));
    }
  }

  // Step 7. Handle post-task
  if (is_v2_) {
    proc.start_task_script_block("xMIN", "0004");
  }
  if (is_3d_task_ && !isnan(proc.curve_engraving_data->safe_height)) {
    NamedArgs args = NamedArgs().rz(proc.curve_engraving_data->safe_height);
    if (proc.z_premove_.speed) {
      args.f = proc.z_premove_.speed;
    }
    proc.moveto(args);
    // Clear curve engraving data to avoid z moving when homing
    proc.clear_curve_engraving_data();
  }
  if (is_rotary_task_) {
    if (is_v2_) {
      if (config_.enable_rotary_z_move) {
        proc.moveto(NamedArgs().rz(1));
      }
      proc.moveto(NamedArgs().ry(config_.spinning_axis_coord_mm).set_is_travel());
      proc.moveto(NamedArgs().ry(config_.home_pos.y()).set_is_travel().set_force_y());
      proc.sync_grbl_motion(36);
      proc.set_a_mode(false);
      backToHome();
    } else {
      proc.moveto(NamedArgs().rx(0).ry(config_.spinning_axis_coord_mm).set_is_travel());
    }
    if (config_.rotary_y_ratio != 1) {
      proc.set_rotary_y_ratio(1);
    }
  } else {
    backToHome();
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
    proc.write_post_config(post_config_);
  }
  proc.terminated();
  qInfo() << "[Export] Took " << t.elapsed() << " milliseconds";
  return true;
}

void ToolpathExporterFcode::convertLayer() {
  qInfo() << "[Export] convert layer: " << current_layer_->name();
  layer_repeat_ = current_layer_->repeat();
  total_repeat_times_ = layer_repeat_;  // For processing progress
  processed_repeat_times_ = 0;
  element_cnt_[0] = 0, element_cnt_[1] = 0;
  if (!current_layer_->isVisible() || total_repeat_times_ == 0) {
    progress_++;
    onProgressChanged(0, true);
    return;
  }

  current_layer_id_++;
  layer_module_ = support_info.MODULES ? LayerModule(current_layer_->module())
                                       : LayerModule::UNIVERSAL_LASER;
  is_printing_layer_ = is_printing_module(layer_module_);
  is_uv_layer_ = is_uv_module(layer_module_);
  is_laser_layer_ = !is_printing_layer_ && !is_uv_layer_;
  layer_pwm_scale_ = 1 - current_layer_->minPower() / current_layer_->power();
  if (layer_pwm_scale_ <= 0) {
    layer_pwm_scale_ = 1;
  }
  layer_speed_ = qMax(is_printing_layer_ ? current_layer_->printingSpeed()
                                         : current_layer_->speed(),
                      config_.min_speed) * 60;  // mm/min
  if (is_3d_task_ && config_.curve_speed_limit > 0 &&
      layer_speed_ > config_.curve_speed_limit) {
    layer_speed_ = config_.curve_speed_limit;
  }
  layer_path_speed_ = layer_speed_;
  if (config_.vector_speed_limit > 0 &&
      layer_path_speed_ > config_.vector_speed_limit) {
    layer_path_speed_ = config_.vector_speed_limit;
  }
  layer_backlash_ = config_.enable_custom_backlash
                        ? current_layer_->xBacklash()
                        : get_backlash_compensation(hardware_, layer_speed_, config_.expected_module);
  if (is_3d_task_) {
    // curve_engraving_z_speed_limit
    double curve_z_limit = current_layer_->ceZLimit();
    if (curve_z_limit > 0) {
      QString key = "z_speed_limit";
      proc.set_curve_engraving_data_by_key(key, curve_z_limit);
    }
  }

  if (is_v2_) {
    proc.write_string("TASK", 4);
  }
  // transition script
  if (is_v2_) {
    proc.start_task_script_block("TRAN", nullptr);
    if (support_info.MODULES) {
      if (support_info.MODULE_TRANSITION) {
        if (is_rotary_task_ && config_.enable_rotary_z_move) {
          proc.moveto(NamedArgs().rz(1));
        }
        QPointF tran_pos;
        if (has_job_origin_) {
          tran_pos = QPointF(0, 0);
        } else if (!hw_profile.tran_pos.isNull()) {
          tran_pos = hw_profile.tran_pos;
        } else {
          tran_pos = QPointF(hw_profile.width / 2, hw_profile.length / 2);
        }
        if (is_rotary_task_ && config_.enable_rotary_z_move) {
          proc.moveto(NamedArgs().ry(config_.home_pos.y()).set_is_travel().set_force_y());
          proc.moveto(NamedArgs().rx(tran_pos.x()).set_is_travel().set_force_y());
          proc.moveto(NamedArgs().ry(tran_pos.y()).set_is_travel().set_force_y());
        } else {
          proc.moveto(NamedArgs().rx(tran_pos.x()).ry(tran_pos.y()).set_is_travel().set_force_y());
          proc.sync_motion_type2(179, 3.0);
        }
      }
      proc.sync_grbl_motion(0);
      if (support_info.MODULE_TRANSITION) {
        proc.flux_custom_cmd(168);
        proc.flux_custom_cmd(174);
      }
      proc.user_selection_cmd(0);
      if (support_info.MODULE_TRANSITION) {
        if (!has_job_origin_) {
          proc.grbl_system_cmd(0);
        }
        proc.sync_grbl_motion(0);
      }
      proc.miscellaneous_cmd(0);
    }
    proc.end_task_script_block();
  }

  if (support_info.MODULES) {
    layer_offset_ = config_.module_offsets.value(layer_module_);
  } else if (config_.enable_diode && current_layer_->isUseDiode()) {
    layer_offset_ = config_.diode_offset;
  } else {
    layer_offset_ = QPointF(0, 0);
  }

  if (is_v2_) {
    proc.start_task_script_block("MAIN", nullptr);
    if (is_rotary_task_) {
      if (config_.enable_rotary_z_move) {
        proc.set_rotary_wait_move(true, config_.spinning_axis_coord_mm - layer_offset_.y());
      } else {
        proc.moveto(NamedArgs()
                        .ry(config_.spinning_axis_coord_mm - layer_offset_.y())
                        .set_is_travel()
                        .set_force_y());
      }
      layer_offset_.setY(0);
    } else {
      proc.sync_motion_type2(179, 2.0);
    }
    proc.sync_grbl_motion(0);
    proc.miscellaneous_cmd(1);
  }
  if (config_.enable_diode) {
    proc.set_toolhead_laser_module(current_layer_->isUseDiode());
  }

  layer_clip_ = getClipRect(config_.workarea_clip, layer_offset_, layer_module_, is_rotary_task_);
  if (has_job_origin_) {
    // perform after clip rect calculation
    layer_offset_ += config_.job_origin;
  }

  float layer_height = current_layer_->targetHeight();
  float focus = current_layer_->focus();
  float focus_step = current_layer_->focusStep();
  bool has_focus_adjust = support_info.REL_Z_MOVE && !is_printing_layer_ &&
                          (focus > 0 || focus_step > 0);
  if (has_focus_adjust && focus > 0) {
    proc.sync_motion_type2(184, focus);
  } else if (config_.enable_autofocus && layer_height > 0) {
    if (!did_home_z_) {
      homeZAxis();
      did_home_z_ = true;
    }
  }

  proc.set_toolhead_pwm(-current_layer_->power() / 100);

  int laser_air_assist = -1;  // -1: not set (printing, uv)
  proc.set_is_main_task(true);
  if (is_printing_layer_ || is_uv_layer_) {
    layer_color_ = get_color(current_layer_->color().name());
    convertPrintingLayer();
  } else {
    preprocessLaserLayer();

    if (support_info.LASER_DELAY) {
      int laser_delay = current_layer_->laserDelay();
      if (laser_delay == 0) {
        laser_delay = get_laser_delay(hardware_, config_.watt, layer_speed_ / 60.0);
      }
      if (laser_delay > 0) {
        proc.sync_motion_type2(153, laser_delay);
      }
    }
    laser_air_assist = current_layer_->airAssist();
    float z_step = current_layer_->stepHeight();
    for (int r = 0; r < layer_repeat_; r++) {
      processed_repeat_times_ = r;  // For processing progress
      if (has_focus_adjust && focus_step > 0 && r > 0) {
        proc.sync_motion_type2(184, focus_step);
      } else if (config_.enable_autofocus && layer_height > 0) {
        float target_z = 17.0 - layer_height - config_.z_offset + r * z_step;
        target_z = round(qMax(0.0f, qMin(17.0f, target_z)) * 100) / 100;
        proc.moveto(NamedArgs().rz(target_z));
      }
      convertLaserLayer();
      proc.set_toolhead_pwm(0);
    }
    proc.moveto(NamedArgs().rs(0));
    if (has_focus_adjust && focus_step > 0 && layer_repeat_ > 1) {
      float total_moved = (layer_repeat_ - 1) * focus_step;
      proc.sync_motion_type2(184, -total_moved);
    }
  }
  proc.set_is_main_task(false);

  if (has_focus_adjust && focus > 0) {
    proc.sync_motion_type2(184, -focus);
  }
  if (is_v2_) {
    proc.end_task_script_block();
  }
  if (is_v2_) {
    // Write task info
    QJsonObject submodule{{"color", "None"}};
    QString submodule_type = "None";
    if (layer_module_ == LayerModule::PRINTER) {
      submodule_type = "Solvent";
      submodule["color"] = COLOR_NAME_MAP.value(layer_color_, "black");
    }
    submodule["type"] = submodule_type;
    QJsonObject task_info{
        {"idx", current_layer_id_},
        {"name", convertUnicode(current_layer_->name())},
        // Note: Force head_type to 4C for UV layer
        {"head_type", int(is_uv_layer_ ? LayerModule::PRINTER_4C : layer_module_)},
        {"display_color", current_layer_->color().name().toUpper()},
        {"submodule", submodule}};
    proc.write_task_info(task_info);
    writePreviewImage();
    // Update post script
    bool need_transition = false;
    if (support_info.PRINTING_SCRIPTS) {
      if (last_module_ == LayerModule::NONE) {
        // first layer
        need_transition = true;
      } else if (last_module_ != layer_module_) {
        need_transition = true;
      }
    } else if ((last_module_ != layer_module_) ||
               (last_module_ == LayerModule::PRINTER &&
                (last_color_ != layer_color_ ||
                 last_sub_type_ != submodule_type))) {
      need_transition = true;
    }
    QJsonObject layer_post_config = {
        {"idx", current_layer_id_},
        {"tran", need_transition ? 1 : 0},
        {"feedrate", std::round(layer_speed_ / 6) / 10},  // mm/s with one decimal place
    };
    if (laser_air_assist >= 0) {
      layer_post_config.insert("air_assist", laser_air_assist * 10);
    }
    post_config_.append(layer_post_config);
    last_module_ = layer_module_;
    last_color_ = layer_color_;
    last_sub_type_ = submodule_type;
  }
}

void ToolpathExporterFcode::preprocessLaserLayer() {
  /*
  Note: For laser layers, objects are processed in the following order:
    1. (first layer) paths: sorted and optimized by PathUtils
    2. (second layer) filled paths: draw all paths with one factory and handled
  by get_bounding_boxes
    3. (second layer) bitmaps: sorted in reverse order, each bitmap handled
  separately
  */

  layer_is_high_quality_ = current_layer_->isHighQuality() && hardware_ == HardwareType::RF;
  int dpmm_y = current_layer_->dpmm() > 0 ? current_layer_->dpmm()
                                          : 10;  // fallback to medium
  int dpmm_x = qMin(dpmm_y, hw_profile.max_pixel_per_mm_x);

  int kernel_size = dpmm_y >= 10 ? std::round(2 * config_.engraving_erode * dpmm_y) + 1 : 0;
  if (kernel_size > 1) {
    kernel_ = cv::getStructuringElement(cv::MORPH_ELLIPSE,
                                        cv::Size(kernel_size, kernel_size));
  } else {
    kernel_.release();
  }

  FactoryKwargs kwargs;
  kwargs.workspaces = &workspaces_;
  kwargs.proc = &proc;
  kwargs.offset = layer_offset_;
  kwargs.pixel_per_mm = dpmm_y;
  kwargs.work_area_mm = work_area_mm_;
  kwargs.clip_rect_mm = layer_clip_;
  kwargs.onProgressChanged = [this](double v, bool a) {
    this->onProgressChanged(v, a);
  };
  kwargs.split_bbox = config_.enable_segmentation;
  kwargs.one_way = current_layer_->isOneWayEngraving() ||
                   (config_.enable_diode && current_layer_->isUseDiode() &&
                    config_.is_diode_one_way_engraving);
  kwargs.fg_pwm_limit = hw_profile.fg_pwm_limit;

  // Note: convert path without dpmm_x
  laser_path_factory_ = std::make_unique<LaserPathFactory>(kwargs);
  laser_path_factory_->set_loop_compensation(config_.loop_compensation);
  laser_path_factory_->set_use_ga(config_.use_ga_reorder);
  kwargs.pixel_per_mm_x = dpmm_x;
  factory_ = std::make_unique<LaserBitmapFactory>(kwargs);
  laser_filled_factory_ = std::make_unique<LaserBitmapFactory>(kwargs);
  laser_filled_factory_->set_default_workspace(1);

  setTransform();
  laser_bitmaps_.clear();
  // First pass: Generate path list and bitmap list and draw filled path with
  // factory
  convert_target_ = ConvertTarget::NON_BITMAP;
  for (auto& shape : current_layer_->children()) {
    convertShape(shape);
  }
  if (current_layer_2_) {
    current_layer_ = current_layer_2_;
    for (auto& shape : current_layer_->children()) {
      convertShape(shape);
    }
  }

  // Note: do this only for filled paths, bitmaps will be handled when
  // converting
  auto workspace = laser_filled_factory_->get_workspace();
  if (!workspace->get_dirty_area().isEmpty()) {
    dilateBinaryBitmap(workspace->get_bitmap());
  }

  if (this->cancelled_)
    return;

  element_cnt_[0] = laser_path_factory_->get_size();
  if (laser_filled_factory_->is_workspace_valid()) {
    element_cnt_[1] += 1;
  }
  total_element_cnt_ = element_cnt_[0] + element_cnt_[1];

  onProgressChanged(0.05, true);
}

void ToolpathExporterFcode::convertLaserLayer() {
  convert_target_ = ConvertTarget::NON_BITMAP;
  // Part 1: Generate path fcode
  outputLayerPathFcode();
  if (this->cancelled_)
    return;
  onProgressChanged(0.05 + 0.95 * element_cnt_[0] / total_element_cnt_, true);

  // Part 2: Generate filled path fcode
  outputBitmapFcode();
  if (this->cancelled_)
    return;
  onProgressChanged(
      0.05 + 0.95 * (element_cnt_[0] + element_cnt_[1]) / total_element_cnt_,
      true);

  // Second pass
  // Part 3: Generate bitmap fcode
  convert_target_ = ConvertTarget::BITMAP_ONLY;
  for (auto& shape : laser_bitmaps_) {
    // Note: Bitmap list is already reversed for first-depth shapes
    // When converting group, reverse order of children and ignore paths
    convertShape(shape);
  }
}

void ToolpathExporterFcode::outputLayerPathFcode() {
  if (laser_path_factory_->get_size() == 0) {
    return;
  }
  AccelerationData acc_override_object = config_.path_acc;
  if (!isnan(config_.z_acc)) {
    acc_override_object.is_valid = true;
    acc_override_object.z = config_.z_acc;
  }
  if (acc_override_object.is_valid) {
    qInfo() << "Set path acc" << acc_override_object.x << acc_override_object.y << acc_override_object.z << acc_override_object.a;
    proc.set_acceleration_override(acc_override_object.x, acc_override_object.y,
                                   acc_override_object.z, acc_override_object.a);
  }
  proc.set_travel_speed(config_.path_travel_speed);
  laser_path_factory_->generate_task_code(layer_path_speed_);
  proc.set_travel_speed(config_.travel_speed);
  // Reset path_acc
  if (acc_override_object.is_valid) {
    proc.sync_grbl_motion(151);
    proc.set_time_est_acc(config_.padding_acc);
  }
}

void ToolpathExporterFcode::outputBitmapFcode() {
  BaseBitmapFactory* factory = convert_target_ == ConvertTarget::NON_BITMAP
                                   ? laser_filled_factory_.get()
                                   : factory_.get();
  if (!factory->is_workspace_valid()) {
    return;
  }
  auto workspace = factory->get_workspace();
  if (workspace->get_dirty_area().isEmpty()) {
    return;
  }
  float padding_acc = config_.padding_acc;
  AccelerationData acc_override_object;
  if (config_.fill_acc.is_valid) {
    acc_override_object = config_.fill_acc;
  } else if (hardware_ == HardwareType::RF) {
    if (layer_is_high_quality_) {
      // 0.8G
      acc_override_object.is_valid = true;
      acc_override_object.x = 8000;
      acc_override_object.y = 2000;
    } else if (layer_speed_ > 500 * 60 && layer_speed_ <= 1200 * 60) {
      // 2.5G
      acc_override_object.is_valid = true;
      acc_override_object.x = 25000;
      acc_override_object.y = 2000;
    }
  }
  if (!isnan(config_.z_acc)) {
    acc_override_object.is_valid = true;
    acc_override_object.z = config_.z_acc;
  }
  if (acc_override_object.is_valid) {
    qInfo() << "Set fill acc" << acc_override_object.x << acc_override_object.y << acc_override_object.z << acc_override_object.a;
    proc.set_acceleration_override(acc_override_object.x, acc_override_object.y,
                                   acc_override_object.z, acc_override_object.a);
    if (!isnan(acc_override_object.x)) {
      padding_acc = acc_override_object.x;
      proc.set_time_est_acc(padding_acc);
    }
  }

  double min_padding = isnan(config_.min_engraving_padding)
                           ? get_default_min_padding(hardware_, layer_module_, config_.expected_module, layer_is_high_quality_)
                           : config_.min_engraving_padding;

  double padding_dist =
      get_padding_dist(min_padding, layer_speed_ / 60, padding_acc);
  double s_curve_padding = NAN;
  if (config_.enable_s_curve) {
    s_curve_padding = get_s_curve_padding_dist(hardware_, layer_speed_ / 60.0);
    if (!isnan(s_curve_padding)) {
      qInfo() << "Enable S-Curve with padding distance:" << s_curve_padding << "mm";
      padding_dist = s_curve_padding;
      proc.sync_motion_type2(156, 1);
      auto s_curve_params = get_s_curve_parameters(hardware_, layer_speed_ / 60.0);
      if (s_curve_params) {
        proc.set_s_curve_params(s_curve_params->a0, s_curve_params->a_max, s_curve_params->jerk);
        proc.set_s_curve_enabled(true);
      }
    }
  }

  GenerateTaskKwargs task_kwargs;
  task_kwargs.support_fast_gradient = config_.enable_fast_gradient;
  task_kwargs.reverse_y = config_.is_reverse_engraving;
  task_kwargs.speed = layer_speed_;
  task_kwargs.mock_fast_gradient = config_.enable_mock_fast_gradient;
  task_kwargs.padding_dist = padding_dist;
  task_kwargs.backlash = layer_backlash_;
  task_kwargs.pwm_scale = layer_pwm_scale_;
  factory->generate_task_code(task_kwargs);

  if (acc_override_object.is_valid) {
    // Reset fill_acc
    proc.sync_grbl_motion(151);
    proc.set_time_est_acc(config_.padding_acc);
  }
  if (!isnan(s_curve_padding)) {
    proc.sync_motion_type2(156);
    proc.set_s_curve_enabled(false);
  }
}

void ToolpathExporterFcode::convertPrintingLayer() {
  // Note: For printing-like layers, objects are drawn on same canvas and
  // handled all together
  convert_target_ = ConvertTarget::ALL;
  // Overwrite repeat, count progress as a whole
  processed_repeat_times_ = 0, total_repeat_times_ = 1;

  // Initialize Factory
  HalftoneParams halftone_params;
  halftone_params.smoother = current_layer_->smooth();
  int halftone = current_layer_->halftone();
  if (halftone > 1) {
    halftone = 2;
    halftone_params.density = current_layer_->amDensity();
  }
  FactoryKwargs kwargs;
  kwargs.workspaces = &workspaces_;
  kwargs.work_area_mm = work_area_mm_;
  kwargs.clip_rect_mm = layer_clip_;
  kwargs.proc = &proc;
  kwargs.onProgressChanged = [this](double v, bool a) {
    this->onProgressChanged(v, a);
  };
  kwargs.offset = layer_offset_;
  kwargs.one_way = config_.is_one_way_printing;
  kwargs.halftone = halftone;
  kwargs.halftone_params = &halftone_params;
  kwargs.split_bbox = config_.enable_segmentation;
  if (is_printing_layer_) {
    prespray_module_ = layer_module_;
    if (layer_module_ == LayerModule::PRINTER_4C) {
      halftone_params.color_multipliers = {
          {PrintingColor::CYAN, current_layer_->cRatio() / 100},
          {PrintingColor::MAGENTA, current_layer_->mRatio() / 100},
          {PrintingColor::YELLOW, current_layer_->yRatio() / 100},
          {PrintingColor::BLACK, current_layer_->kRatio() / 100},
      };
      factory_ = std::make_unique<PrinterBitmapFactory4C>(kwargs);
      if (hw_profile.reverse_4c) {
        factory_->set_reversed(true);
      }
      if (macros) {
        factory_->set_macros(macros);
      }
      factory_->set_am_angle_map(current_layer_->rawAmAngleMap());
      factory_->set_color_curves_map(current_layer_->rawColorCurvesMap());
      factory_->set_refresh_interval(current_layer_->refreshInterval());
      factory_->set_refresh_threshold(current_layer_->refreshThreshold());
      factory_->set_nozzle_mode(current_layer_->nozzleMode());
      factory_->set_nozzle_offset(NozzleMode::RIGHT,
                                  QPointF(current_layer_->nozzleOffsetX(),
                                          current_layer_->nozzleOffsetY()));
      factory_->set_refresh_x_mm(config_.prespray.x());
    } else {
      halftone_params.multiplier = current_layer_->printingStrength() / 100;
      kwargs.color_curve = COLOR_CURVES_MAP[halftone - 1][layer_color_];
      halftone_params.angle = AM_ANGLE_MAP.value(layer_color_, 75);
      factory_ = std::make_unique<PrinterBitmapFactory>(kwargs);
    }
    factory_->set_slice_width(config_.printing_slice_width);
    factory_->set_slice_height(config_.printing_slice_height);
    factory_->set_slice_top_padding(config_.printing_top_padding);
    factory_->set_slice_bot_padding(config_.printing_bot_padding);
  } else if (is_uv_layer_) {
    kwargs.interpolation = current_layer_->interpolation();
    factory_ = std::make_unique<UVBitmapFactory>(kwargs);
    if (layer_module_ == LayerModule::WHITE_INK) {
      factory_->set_uv_type(UVType::WHITE_INK);
    } else if (layer_module_ == LayerModule::VARNISH) {
      factory_->set_uv_type(UVType::VARNISH);
    }
    factory_->set_uv_x_step(current_layer_->uvXStep());
    factory_->set_uv_light_strength(current_layer_->uvStrength());
    if (current_layer_->uvCuringAfter() > 0) {
      factory_->set_uv_curing_after(true);
      int uv_printing_repeat = current_layer_->uvPrintingRepeat();
      int uv_curing_repeat = current_layer_->uvCuringRepeat();
      if (uv_printing_repeat != 1) {
        factory_->set_printing_repeat(uv_printing_repeat);
      }
      factory_->set_uv_curing_repeat(uv_curing_repeat);
    }
  } else {
    Q_ASSERT_X(false, "ToolpathExporterFcode::convertPrintingLayer",
               "Should be printing or UV layer");
  }

  // Add image
  setTransform();
  for (auto& shape : current_layer_->children()) {
    convertShape(shape);
  }
  if (this->cancelled_)
    return;
  element_cnt_[1] = 1;

  // Generate task
  proc.enter_printer_mode();
  float ink = current_layer_->ink();
  int multipass = current_layer_->multipass();
  float black_ratio, actual_saturation;
  if (layer_module_ == LayerModule::PRINTER_4C) {
    // for 4c, saturation 100 means saturation 3 in printer
    black_ratio = 3 * ink / 100.0;
    if (config_.enable_multipass_compensation) {
      QVector<double> extra_factors = {0.6,  0.85, 1,    1.15, 1.3,
                                       1.43, 1.55, 1.65, 1.75, 1.85};
      double extra_factor_4c = extra_factors[qMin(multipass - 1, 9)];
      black_ratio = black_ratio / multipass * extra_factor_4c;
    }
    actual_saturation = 1;
  } else if (is_uv_layer_) {
    actual_saturation = 1;
    // ink: 0 ~ 100
    black_ratio = qMin(ink / (config_.enable_multipass_compensation ? multipass : 1) / 100, 1.0f);
  } else if (config_.enable_multipass_compensation) {
    actual_saturation = qMin(int(std::ceil(ink / multipass)), 9);
    black_ratio = qMin(ink / (multipass * actual_saturation), 1.0f);
  } else {
    actual_saturation = ink;
    black_ratio = 1;
  }

  if (layer_module_ == LayerModule::PRINTER) {
    // only need for printer
    QByteArray nozzle_settings_payload = generate_nozzle_setting_payload(
        actual_saturation, config_.nozzle_voltage, config_.nozzle_pulse_width);
    proc.write_printer_packet(17, nozzle_settings_payload);
  }

  double right_padding = is_uv_layer_ ? current_layer_->rightPadding() : 0;
  double min_padding;
  if (!isnan(config_.min_printing_padding)) {
    min_padding = config_.min_printing_padding;
  } else {
    min_padding = get_default_min_padding(hardware_, layer_module_, config_.expected_module);
  }

  GenerateTaskKwargs printing_kwargs;
  printing_kwargs.reverse_y = config_.is_reverse_engraving;
  printing_kwargs.multipass = multipass;
  printing_kwargs.black_ratio = black_ratio;
  printing_kwargs.repeat = layer_repeat_;
  printing_kwargs.speed = layer_speed_;
  printing_kwargs.padding_dist =
      get_padding_dist(min_padding, layer_speed_ / 60, config_.padding_acc);
  printing_kwargs.padding_dist_right =
      get_padding_dist(right_padding, layer_speed_ / 60, config_.padding_acc);
  factory_->generate_task_code(printing_kwargs);
  proc.exit_printer_mode();
  proc.set_toolhead_pwm(0);
  proc.moveto(NamedArgs().rs(0));
}

void ToolpathExporterFcode::outputPrintingTestFcode() {
  if (support_info.PRINTING_SCRIPTS) {
    if (has_printing_task_) {
      // 0002: pure prespray task
      if (!config_.skip_prespray && hasattr(macros, MacroFunc::test_cartridge)) {
        proc.start_task_script_block("xMIN", "0002");
        macros->test_cartridge();
        proc.end_task_script_block();
      }
      // 0005: task before printing
      if (hasattr(macros, MacroFunc::remove_printer_lid)) {
        proc.start_task_script_block("xMIN", "0005");
        macros->remove_printer_lid();
        proc.end_task_script_block();
      }
      // 0006: task after printing
      if (hasattr(macros, MacroFunc::put_back_printer_lid)) {
        proc.start_task_script_block("xMIN", "0006");
        macros->put_back_printer_lid();
        proc.end_task_script_block();
      }
      // 0007: pure extend table
      if (hasattr(macros, MacroFunc::go_to_standby_pos)) {
        proc.start_task_script_block("xMIN", "0007");
        macros->go_to_standby_pos();
        if (hasattr(macros, MacroFunc::extend_table)) {
          macros->extend_table();
        }
        proc.end_task_script_block();
      }
    }
  } else {
    // printing test, prespray task
    if (is_v2_ && prespray_module_ != LayerModule::NONE && !config_.prespray.isEmpty()) {
      QPointF offset = config_.module_offsets[prespray_module_];
      proc.start_task_script_block("xMIN", "0001");
      if (!has_job_origin_) {
        proc.grbl_system_cmd(0);
      }
      InwardRect clip_rect = getClipRect(InwardRect(), offset, prespray_module_);
      PresprayParams prespray_params_test;
      prespray_params_test.work_area_mm = work_area_mm_;
      prespray_params_test.module = prespray_module_;
      prespray_params_test.prespray = config_.prespray;
      prespray_params_test.clip_rect_mm = clip_rect;
      prespray_params_test.module_offsets = config_.module_offsets;
      prespray_params_test.travel_speed = config_.travel_speed;
      prespray_params_test.do_test = true;
      prespray_params_test.has_job_origin = has_job_origin_;
      prespray_params_test.job_origin = config_.job_origin;
      prespray_params_test.is_rotary_task = is_rotary_task_;
      prespray_params_test.rotary_z_motion = config_.enable_rotary_z_move;
      prespray_params_test.reverse_4c = hw_profile.reverse_4c;
      generate_prespray_code(proc, prespray_params_test);
      proc.end_task_script_block();
      // 0002 pure prespray task
      proc.start_task_script_block("xMIN", "0002");
      PresprayParams prespray_params_pure;
      prespray_params_pure.work_area_mm = work_area_mm_;
      prespray_params_pure.module = prespray_module_;
      prespray_params_pure.prespray = config_.prespray;
      prespray_params_pure.clip_rect_mm = clip_rect;
      prespray_params_pure.module_offsets = config_.module_offsets;
      prespray_params_pure.travel_speed = config_.travel_speed;
      prespray_params_pure.do_test = false;
      prespray_params_pure.has_job_origin = has_job_origin_;
      prespray_params_pure.job_origin = config_.job_origin;
      prespray_params_pure.is_rotary_task = is_rotary_task_;
      prespray_params_pure.rotary_z_motion = config_.enable_rotary_z_move;
      prespray_params_pure.reverse_4c = hw_profile.reverse_4c;
      generate_prespray_code(proc, prespray_params_pure);
      proc.end_task_script_block();
    }
  }
}

void ToolpathExporterFcode::writePreviewImage() {
  // Note: preview image is not handled right now
  QImage dummy_preview_bitmap = QImage(1, 1, QImage::Format_ARGB32);
  dummy_preview_bitmap.fill(Qt::white);
  QByteArray byteArray;
  QBuffer buffer(&byteArray);
  dummy_preview_bitmap.save(&buffer, "PNG");
  proc.write_string("PREV", 4);
  proc.write_string(byteArray.data(), byteArray.size(), true);
}

bool ToolpathExporterFcode::convertShape(const ShapePtr& shape,
                                         bool from_group) {
  const PathShape* path;
  // Add bitmap or group with bitmap to laser_bitmaps_
  bool has_bitmap = false;
  switch (shape->type()) {
    case Shape::Type::Group:
      has_bitmap = convertGroup(dynamic_cast<GroupShape*>(shape.get()));
      if (has_bitmap && !from_group && convert_target_ == ConvertTarget::NON_BITMAP) {
        laser_bitmaps_.prepend(shape);
      }
      break;
    case Shape::Type::Bitmap:
      has_bitmap = true;
      if (convert_target_ == ConvertTarget::NON_BITMAP) {
        element_cnt_[1]++;
        if (!from_group) {
          laser_bitmaps_.prepend(shape);
        }
      } else {
        convertBitmap(dynamic_cast<BitmapShape*>(shape.get()));
      }
      break;
    case Shape::Type::Path:
    case Shape::Type::Text:
      if (convert_target_ != ConvertTarget::BITMAP_ONLY) {
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
  if (convert_target_ == ConvertTarget::BITMAP_ONLY) {
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
  QTransform transform = global_transform_ * factory_->get_transform();
  QRectF new_dirty_area = transform.mapRect(bmp->boundingRect());
  transform = bmp->transform() * transform;
  QImage transformed_image =
      bmp->sourceImage()
          .transformed(transform, bmp->gradient() ? Qt::SmoothTransformation
                                                  : Qt::FastTransformation)
          .convertToFormat(QImage::Format_ARGB32);
  if (bmp->gradient()) {
    if (!is_laser_layer_) {
      if (layer_module_ == LayerModule::PRINTER_4C) {
        factory_->add_image_by_color(transformed_image, new_dirty_area, bmp->color());
      } else {
        factory_->add_image(transformed_image, new_dirty_area);
      }
    } else if (bmp->pwm()) {
      if (!config_.enable_fast_gradient) {
        clearTransparent(&transformed_image);
        transformed_image = transformed_image.convertToFormat(QImage::Format_Mono)
                .convertToFormat(QImage::Format_Grayscale8);
      }
      factory_->add_image(transformed_image, new_dirty_area);
    } else {
      clearTransparent(&transformed_image);
      ImageSharpenDialog sharpener = ImageSharpenDialog();
      sharpener.loadImage(transformed_image);
      sharpener.onSharpnessChanged(1);
      sharpener.onRadiusChanged(2);
      transformed_image = sharpener.getSharpenedImage()
                                   .convertToFormat(QImage::Format_Mono,
                                                    Qt::MonoOnly | Qt::DiffuseAlphaDither)
                                   .convertToFormat(QImage::Format_Grayscale8);
      factory_->add_image(transformed_image, new_dirty_area);
    }
  } else if (is_laser_layer_) {
    transformed_image = imageBinarize(&transformed_image, bmp->thrsh_brightness());
    dilateBinaryBitmap(&transformed_image);
    factory_->add_image(transformed_image, new_dirty_area);
  } else {
    // Note: use ARGB version to prevent transparent converted to white and
    // clear previous data
    imageBinarizeARGB32(&transformed_image, bmp->thrsh_brightness());
    factory_->add_image(transformed_image, new_dirty_area);
  }
  if (convert_target_ == ConvertTarget::BITMAP_ONLY) {
    factory_->set_pwm_engraving(config_.enable_pwm && bmp->pwm());
    outputBitmapFcode();
    factory_->get_workspace()->invalidate();
  }
}

void ToolpathExporterFcode::convertPath(const PathShape* path) {
  bool has_filled =
      ((path->isFilled() && current_layer_->type() == Layer::Type::Mixed) ||
       current_layer_->type() == Layer::Type::Fill ||
       current_layer_->type() == Layer::Type::FillLine);
  if (has_filled) {
    QPainterPath transformed_path = (path->transform() * global_transform_ *
                                     laser_filled_factory_->get_transform())
                                        .map(path->path());
    QRectF path_bounding_rect = transformed_path.boundingRect();

    laser_filled_factory_->add_filled_path(transformed_path, path_bounding_rect);
  } else {
    QPainterPath transformed_path = (path->transform() * global_transform_ *
                                     laser_path_factory_->get_transform())
                                        .map(path->path());
    laser_path_factory_->add_path(transformed_path);
  }
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

  for (int y = 0; y < src->height(); ++y) {
    QRgb* ptr = (QRgb*)src->scanLine(y);
    for (int x = 0; x < src->width(); ++x) {
      int alpha = qAlpha(ptr[x]);
      if (alpha == 255) continue;
      // composite with white background
      int gray = qGray(ptr[x]);
      int blended = (gray * alpha + 255 * (255 - alpha)) / 255;
      ptr[x] = qRgba(blended, blended, blended, 255);
    }
  }
}

void ToolpathExporterFcode::dilateBinaryBitmap(QImage* image) {
  if (!is_laser_layer_ || kernel_.empty() || !image || image->isNull()) {
    return;
  }
  cv::Mat cv_image = QImageToMat(*image);
  cv::dilate(cv_image, cv_image, kernel_);
  *image = MatToQImage(cv_image);
}

void ToolpathExporterFcode::homeZAxis() {
  proc.moveto(NamedArgs().rz(-1));
}

void ToolpathExporterFcode::backToHome() {
  proc.moveto(NamedArgs()
                  .rx(config_.home_pos.x())
                  .ry(config_.home_pos.y())
                  .set_is_travel());
}

void ToolpathExporterFcode::handleCancel() {
  this->cancelled_ = true;
  if (factory_)
    factory_->handleCancel();
  if (laser_filled_factory_)
    laser_filled_factory_->handleCancel();
  if (laser_path_factory_)
    laser_path_factory_->handleCancel();
}

/**
 * Update progress and emit signal if necessary
 * Also check if the process is cancelled
 */
void ToolpathExporterFcode::onProgressChanged(double value, bool absolute) {
  // TODO: rewrite progress calculation
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
