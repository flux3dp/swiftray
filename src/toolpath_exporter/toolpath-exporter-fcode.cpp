#include "toolpath-exporter-fcode.h"
#include "toolpath_exporter/factories/laser.h"
#include "toolpath_exporter/factories/printer-4c.h"
#include "toolpath_exporter/factories/printer.h"
#include "toolpath_exporter/factories/uv.h"
#include "toolpath_exporter/macros/beamo2.h"
#include "toolpath_exporter/macros/prespray.h"
#include "toolpath_exporter/macros/uv1.h"
#include "toolpath_exporter/toolpath-utils.h"
#include "windows/image-sharpen-dialog.h"
#include <QBuffer>
#include <QCoreApplication>
#include <QDebug>
#include <QElapsedTimer>
#include <QFontMetrics>
#include <QPainter>
#include <cmath>

// How far a galvo tile's clip reaches past its own edge, mm. Only has to beat
// floating-point noise on a seam; GalvoListWriter drops anything this short.
constexpr double kGalvoSeamEpsilonMm = 1e-6;

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
  if (is_galvo_machine_ && proc.galvo()) {
    proc.galvo()->params() = config_.galvo_params;
    proc.galvo()->set_max_commands_per_list(config_.galvo_max_list_commands);
  }

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
  is_galvo_machine_ = hardware_ == HardwareType::HEXA2;
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
  config_.burst_refresh = param["burst_refresh"].toBool();
  config_.prespray_times = param["prespray_times"].toInt(3);
  config_.color_order = param["ico"].toString();
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

  if (is_galvo_machine_) {
    // Galvo list parameters. Optical calibration, scanahead and the head type
    // are deliberately absent: they never travel in the fcode (§19.6), the
    // player pushes them out of band from the machine profile.
    config_.galvo_field_mm = param["galvo_field"].toDouble(110);
    // The block displacement follows the lens; a caller that has a better
    // number for its own machine sends galvo_block and overrides this.
    config_.galvo_block_size = get_galvo_block_size(config_.galvo_field_mm);
    config_.galvo_travel_speed = param["galvo_ts"].toDouble(3000);
    config_.galvo_debug_image = param["galvo_debug_image"].toString();
    if (param.contains("galvo_block")) {
      const QJsonArray block = param["galvo_block"].toArray();
      if (block.size() == 2) {
        config_.galvo_block_size =
            QSizeF(block[0].toDouble(), block[1].toDouble());
      }
    }
    GalvoParams& g = config_.galvo_params;
    g.jump_speed_mm_s = param["galvo_jump_speed"].toDouble(4000);
    g.laser_on_delay_us = param["galvo_laser_on_delay"].toDouble(-100);
    g.laser_off_delay_us = param["galvo_laser_off_delay"].toDouble(100);
    g.scanner_mark_delay_us = param["galvo_scanner_mark_delay"].toDouble(100);
    g.scanner_polygon_delay_us =
        param["galvo_scanner_polygon_delay"].toDouble(50);
    // Estimation only; set_delay_mode itself is a CONTROL instruction (§6.2).
    g.jump_delay_min_us = param["galvo_jump_delay_min"].toDouble(200);
    g.jump_delay_max_us = param["galvo_jump_delay_max"].toDouble(400);
    g.emit_standby = param["galvo_standby"].toBool(true);
    g.standby_period_us = param["galvo_standby_period"].toDouble(100);
    g.standby_width_us = param["galvo_standby_width"].toDouble(1);
    // opcode 12's second parameter on a Mopa head, where it is not derived
    // from power. 0 is what the execution end has used for this head.
    g.pulse_length_us = param["galvo_mopa_pulse_length"].toDouble(0);
    config_.galvo_max_list_commands = param["galvo_max_list_commands"].toInt(0);
  }

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
    // get_boundary is the strip of travel the module costs the gantry. A galvo
    // then reaches half a field past wherever the gantry can put its lens, so
    // the strip it actually costs the drawing is that much smaller -- often
    // nothing at all, since half a field dwarfs the clearance.
    const double reach =
        is_galvo_machine_ && is_galvo_module(module) ? config_.galvo_field_mm / 2 : 0;
    module_clip.right = qMax(module_clip.right - offset.x() - reach, 0.0);
    module_clip.left = qMax(module_clip.left + offset.x() - reach, 0.0);
    if (rotary) {
      module_clip.top = 0;
      module_clip.bottom = 0;
    } else {
      module_clip.top = qMax(module_clip.top + offset.y() - reach, 0.0);
      module_clip.bottom = qMax(module_clip.bottom - offset.y() - reach, 0.0);
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
    if (!config_.color_order.isEmpty()) {
      // 4C ink per cartridge slot, so readers can map payload nibble bits back to inks
      proc.add_metadata("COLOR_ORDER", config_.color_order);
    }

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
  layer_is_galvo_ = is_galvo_machine_ && is_laser_layer_ &&
                    is_galvo_module(layer_module_);
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
        } else if (hw_profile.tran_pos.has_value()) {
          tran_pos = *hw_profile.tran_pos;
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
      if (layer_is_galvo_) {
        convertGalvoLaserLayer();
      } else {
        convertLaserLayer();
      }
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

  layer_texture_enabled_ = current_layer_->hasTexture();
  if (layer_texture_enabled_) {
    layer_texture_params_ = {
        current_layer_->textureMode(),
        current_layer_->textureRandomIntensity(),
        current_layer_->textureStripeAngle(),
        current_layer_->textureStripeInterval(),
        current_layer_->textureStripeIntensity(),
        1.0 / dpmm_x,
        1.0 / dpmm_y,
    };
  }

  // use y to determine auto shrink or not because dpmm_y >= dpmm_x
  int kernel_size_y = dpmm_y >= 10 ? std::round(2 * config_.engraving_erode * dpmm_y) + 1 : 0;
  if (kernel_size_y > 1) {
    int kernel_size_x = dpmm_x >= 10 ? std::round(2 * config_.engraving_erode * dpmm_x) + 1 : 1;
    kernel_ = cv::getStructuringElement(cv::MORPH_ELLIPSE,
                                        cv::Size(kernel_size_x, kernel_size_y));
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
                   layer_is_high_quality_ ||
                   (config_.enable_diode && current_layer_->isUseDiode() &&
                    config_.is_diode_one_way_engraving);
  kwargs.one_way_reversed = current_layer_->isOneWayEngravingReversed();
  if (layer_is_high_quality_ && !current_layer_->isOneWayEngraving()) {
    qInfo() << "High quality: force one-way engraving";
  }
  kwargs.fg_pwm_limit = hw_profile.fg_pwm_limit;

  // Note: convert path without dpmm_x
  laser_path_factory_ = std::make_unique<LaserPathFactory>(kwargs);
  laser_path_factory_->set_loop_compensation(config_.loop_compensation);
  laser_path_factory_->set_use_ga(config_.use_ga_reorder);
  if (layer_is_galvo_) {
    laser_raster_factory_ = std::make_unique<LaserRasterGalvoFactory>(kwargs);
    laser_raster_factory_->set_dotting_time(current_layer_->dottingTime());
    laser_depth_factory_ = std::make_unique<LaserRasterGalvoFactory>(kwargs);
    galvo_depth_bitmaps_.clear();
    laser_hatch_factory_ = std::make_unique<LaserPathFilledFactory>(kwargs);
    laser_hatch_factory_->set_fill_params(
        {current_layer_->fillInterval(), current_layer_->fillAngle(),
         current_layer_->fillBidirectional(),
         current_layer_->fillHatch() ? 2 : 1});
  } else {
    laser_hatch_factory_.reset();
    laser_raster_factory_.reset();
    laser_depth_factory_.reset();
  }
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
    texturizeLaserImage(workspace->get_bitmap(), true);
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

void ToolpathExporterFcode::emitGalvoBlocks(
    const QVector<GalvoBlock>& blocks,
    const std::function<void()>& emit_content,
    double progress_from,
    double progress_to,
    bool centre_when_done) {
  GalvoListWriter* galvo = proc.galvo();
  const double half_field = config_.galvo_field_mm / 2;
  bool anything_emitted = false;
  for (int i = 0; i < blocks.size(); i++) {
    const GalvoBlock& block = blocks[i];
    const bool last = i == blocks.size() - 1;
    const QPointF centre = block.park;
    // Park the gantry, then wait for the motion to finish before anything goes
    // to the galvo board. Without the sync the ok is only an ack, not a
    // completed move (§10 step 2). This runs on the block's first record, so a
    // tile the drawing never reaches costs no head travel -- most of them do
    // not, once a sparse drawing is tiled across the bed.
    galvo->beginBlock(centre, half_field, [this, centre]() {
      proc.moveto(NamedArgs().rx(centre.x()).ry(centre.y()).set_is_travel());
      proc.sync_grbl_motion(0);
    });
    galvo->beginList();
    if (blocks.size() > 1) {
      galvo->set_clip_rect(block.clip);
    }
    emit_content();
    galvo->clear_clip_rect();
    anything_emitted = anything_emitted || galvo->block_started();
    // Only park the mirrors if there was work to park them after; a layer that
    // engraves nothing should not open a list just to say so. The last block
    // may itself be empty, and centring there is what opens it.
    if (last && centre_when_done && anything_emitted) {
      galvo->returnToCentre();
    }
    galvo->endBlock();  // closes the list: DISABLE_LASER + SET_END_OF_LIST(ms)

    // §15-S8: the galvo's own travel belongs in travel_dist too.
    proc.add_travel_dist(galvo->block_distance_mm());
    if (this->cancelled_) {
      return;
    }
    onProgressChanged(progress_from + (progress_to - progress_from) *
                                          (i + 1.0) / blocks.size(),
                      true);
  }
}

void ToolpathExporterFcode::convertGalvoDepthBitmaps() {
  if (galvo_depth_bitmaps_.isEmpty() || !laser_depth_factory_) {
    return;
  }
  // Depth is cut as a stack of binary passes, each one taking a little more of
  // the image than the last, with the head dropping between them -- the same
  // shape as ToolpathExporter::rasterBitmapDepthMode(). It is the gcode
  // treatment rather than the fcode PWM one because a galvo has no per-pixel
  // power to modulate.
  const double span = 0.5 / galvo_depth_bitmaps_.size();
  double progress = 0.5;
  int remaining = galvo_depth_bitmaps_.size();
  for (const BitmapShape* bmp : galvo_depth_bitmaps_) {
    remaining--;
    QTransform transform =
        global_transform_ * laser_depth_factory_->get_transform();
    QRectF bbox_px = transform.mapRect(bmp->boundingRect());
    transform = bmp->transform() * transform;
    QImage image = bmp->sourceImage()
                       .transformed(transform, Qt::SmoothTransformation)
                       .convertToFormat(QImage::Format_ARGB32);
    bbox_px.setWidth(image.width());
    bbox_px.setHeight(image.height());
    // The pixel range decides the thresholds, and it is taken before the
    // transparency is flattened: a transparent corner is not part of the
    // picture and must not drag the lightest value to white. Done here rather
    // than through findMinMaxPixel(), which reads QRgb and so insists on an
    // ARGB32 image that is already grey.
    int darkest = 256;
    int lightest = 0;
    for (int y = 0; y < image.height(); y++) {
      const QRgb* row = reinterpret_cast<const QRgb*>(image.constScanLine(y));
      for (int x = 0; x < image.width(); x++) {
        if (qAlpha(row[x]) == 0) {
          continue;
        }
        const int grey = qGray(row[x]);
        darkest = qMin(darkest, grey);
        lightest = qMax(lightest, grey);
      }
    }
    if (darkest > 255) {
      continue;  // nothing but transparency
    }
    clearTransparent(&image);
    image = image.convertToFormat(QImage::Format_Grayscale8);
    const int passes = qMax(bmp->depthPass(), 1);
    const double z_step = bmp->depthZStep();
    const double threshold_step = double(lightest - darkest) / passes;
    qInfo() << "[Export] Galvo depth bitmap: pixels" << darkest << ".."
            << lightest << "over" << passes << "pass(es), z step" << z_step;

    laser_depth_factory_->clear();
    laser_depth_factory_->add_bitmap(image, bbox_px, false);
    const QVector<GalvoBlock> blocks =
        planGalvoBlocks(laser_depth_factory_->get_bounds_mm());

    bool transposed = false;
    for (int pass = 0; pass < passes; pass++) {
      if (pass != 0 && z_step != 0) {
        // Between passes, and so outside any block: Z never reaches the galvo
        // board (§4.4), and this is the only place the gantry can take it.
        proc.sync_motion_type2(184, -z_step);
      }
      const int threshold =
          passes <= 1 ? (lightest + darkest) / 2
                      : int(lightest - pass * threshold_step);
      emitGalvoBlocks(
          blocks,
          [&]() {
            laser_depth_factory_->generate_task_code(layer_speed_, threshold,
                                                     transposed);
          },
          progress + span * pass / passes,
          progress + span * (pass + 1.0) / passes,
          remaining == 0 && pass == passes - 1);
      if (this->cancelled_) {
        return;
      }
      // Cross the passes over each other, as the gcode depth pass does.
      transposed = !transposed;
    }
    if (passes > 1 && z_step != 0) {
      // Put the head back where the layer left it, so a second depth bitmap
      // does not start out of focus.
      proc.sync_motion_type2(184, z_step * (passes - 1));
    }
    progress += span;
  }
}

void ToolpathExporterFcode::prepareGalvoBitmaps() {
  if (!laser_raster_factory_ || laser_bitmaps_.isEmpty()) {
    return;
  }
  // The same second pass the gantry path runs, except convertBitmap hands the
  // images to the raster factory instead of painting them into a workspace.
  const ConvertTarget previous = convert_target_;
  convert_target_ = ConvertTarget::BITMAP_ONLY;
  setTransform();
  for (auto& shape : laser_bitmaps_) {
    convertShape(shape);
    if (this->cancelled_) {
      break;
    }
  }
  convert_target_ = previous;
}

void ToolpathExporterFcode::applyGalvoLayerParams() {
  GalvoParams& g = proc.galvo()->params();
  g = config_.galvo_params;  // start from the machine baseline every layer
  g.mark_speed_mm_s = layer_path_speed_ / 60;  // mm/min -> mm/s
  g.power_pct = current_layer_->power();
  // Layer frequency is in kHz, so the period is 1000 / f microseconds. This is
  // the same conversion the execution end makes for the text "Q" command.
  int frequency_khz = current_layer_->frequency();
  if (frequency_khz > 0) {
    g.pulse_period_us = 1000.0 / frequency_khz;
  }
  if (layer_module_ == LayerModule::GALVO_MOPA) {
    // The Mopa head carries its own width on opcode 12's third parameter (ns),
    // set once here for the layer. Its second parameter stays at the value the
    // execution end has always used for this head.
    g.derive_pulse_from_power = false;
    g.mopa_pulse_ns = current_layer_->pulseWidth();
    // Standby is pre-ionization, which is a CO2 notion; the Mopa head skips the
    // call, as the text path did before it was removed.
    g.emit_standby = false;
  } else {
    // The CO2 head has no separate power input: its duty cycle is the power, so
    // the writer derives opcode 12's second parameter and re-sends it whenever
    // power moves. The third parameter means nothing here; 1 is the SDK floor.
    g.derive_pulse_from_power = true;
    g.mopa_pulse_ns = 1;
  }
  // Dotting is not a vector-path setting: on the old text path it was armed
  // only around gradient bitmaps. Raster has no galvo path in phase 1 (§19.5),
  // so nothing here ever dots and layer dottingTime() stays unread.
  g.dotting_time_us = 0;
  double wobble_step = current_layer_->wobbleStep();
  double wobble_diameter = current_layer_->wobbleDiameter();
  if (wobble_step > 0 && wobble_diameter > 0) {
    // SDK order is (transversal, longitudinal, space, mode); the execution end
    // passes the diameter for both axes.
    g.wobble_mode = GalvoWobbleMode::WHEEL;
    g.wobble_transversal_mm = wobble_diameter;
    g.wobble_longitudinal_mm = wobble_diameter;
    g.wobble_space_mm = wobble_step;
  } else {
    g.wobble_mode = GalvoWobbleMode::DISABLE;
  }
  g.wobble_k = calculate_wobble_k(wobble_step, wobble_diameter);
}

QRectF ToolpathExporterFcode::galvoHeadTravel() const {
  // get_boundary's margins are what the module costs the gantry, so what is
  // left of the work area is where the head itself can stand.
  const InwardRect b = get_boundary(hardware_, layer_module_);
  const double w = work_area_mm_.width() - b.left - b.right;
  const double h = work_area_mm_.height() - b.top - b.bottom;
  return QRectF(b.left, b.top, qMax(w, 0.0), qMax(h, 0.0));
}

QVector<ToolpathExporterFcode::GalvoBlock> ToolpathExporterFcode::planGalvoBlocks(
    const QRectF& content) const {
  // The gantry cannot stop wherever it likes. A block displacement is a whole
  // count of X and Y full steps, so the head only ever stands on multiples of
  // it measured from the machine origin -- and since the layer frame is the
  // main head's own frame, those multiples are the lattice below.
  //
  // A head owns the displacement-sized cell centred on itself. The galvo
  // reaches half a field either way, which is wider than the displacement, so
  // neighbouring blocks overlap and the outermost ones can stretch past their
  // cell to wherever the beam still reaches.
  QVector<GalvoBlock> blocks;
  if (content.isEmpty()) {
    return blocks;
  }
  const double sx = config_.galvo_block_size.width();
  const double sy = config_.galvo_block_size.height();
  const double half = config_.galvo_field_mm / 2;
  if (sx > config_.galvo_field_mm || sy > config_.galvo_field_mm) {
    // The block displacement is derived from the field for exactly this reason
    // -- a smaller lens needs a smaller one -- so this means they disagree.
    qWarning() << "[Export] galvo block displacement" << config_.galvo_block_size
               << "is wider than the" << config_.galvo_field_mm
               << "mm field; a head cannot reach its own cell";
  }
  if (sx <= 0 || sy <= 0) {
    qWarning() << "[Export] galvo block displacement" << config_.galvo_block_size
               << "is not usable; falling back to one block";
    blocks.append({content, content, content.center()});
    return blocks;
  }
  const QRectF travel = galvoHeadTravel();
  // Lattice points the head can actually stand on.
  const int i_lo = int(std::ceil(travel.left() / sx - kGalvoSeamEpsilonMm));
  const int i_hi = int(std::floor(travel.right() / sx + kGalvoSeamEpsilonMm));
  const int j_lo = int(std::ceil(travel.top() / sy - kGalvoSeamEpsilonMm));
  const int j_hi = int(std::floor(travel.bottom() / sy + kGalvoSeamEpsilonMm));
  if (i_lo > i_hi || j_lo > j_hi) {
    qWarning() << "[Export] no galvo head position on the" << config_.galvo_block_size
               << "lattice lies within the travel" << travel;
    return blocks;
  }
  // Index of the cell a coordinate falls in, since cell k spans k*s +/- s/2.
  auto cell_of = [](double v, double s, int lo, int hi) {
    return qBound(lo, int(std::floor(v / s + 0.5)), hi);
  };
  // Whether to split at all is the field's call, not the block displacement's:
  // a drawing that fits inside one field is done from a single head position,
  // and that position is free. The lattice only governs how a split is laid
  // out, so it does not apply here.
  if (content.width() <= config_.galvo_field_mm &&
      content.height() <= config_.galvo_field_mm) {
    const QPointF park(
        qBound(travel.left(), content.center().x(), travel.right()),
        qBound(travel.top(), content.center().y(), travel.bottom()));
    if (qAbs(content.left() - park.x()) <= half &&
        qAbs(content.right() - park.x()) <= half &&
        qAbs(content.top() - park.y()) <= half &&
        qAbs(content.bottom() - park.y()) <= half) {
      blocks.append({content, content, park});
      return blocks;
    }
  }
  const int i0 = cell_of(content.left(), sx, i_lo, i_hi);
  const int j0 = cell_of(content.top(), sy, j_lo, j_hi);
  // The far edges are nudged inwards first. A cell keeps its right and bottom
  // edges and gives up its left and top, so content ending exactly on a
  // boundary belongs to the cell before it -- ask for the cell at the boundary
  // itself and the answer is a column that owns nothing but that one line.
  const int i1 = qMax(i0, cell_of(content.right() - kGalvoSeamEpsilonMm, sx,
                                  i_lo, i_hi));
  const int j1 = qMax(j0, cell_of(content.bottom() - kGalvoSeamEpsilonMm, sy,
                                  j_lo, j_hi));
  blocks.reserve((i1 - i0 + 1) * (j1 - j0 + 1));
  for (int j = j0; j <= j1; j++) {
    for (int c = i0; c <= i1; c++) {
      // Serpentine across rows so the head does not fly back every row.
      const int i = (j - j0) % 2 == 0 ? c : (i0 + i1 - c);
      const QPointF park(i * sx, j * sy);
      // Once split, a block engraves its own cell and no more, even though the
      // field reaches further. The head sits at the cell's centre.
      const QRectF region(park.x() - sx / 2, park.y() - sy / 2, sx, sy);
      // Geometry lying along a seam goes to the tile above or to the left of
      // it. A tile therefore reaches just past its right and bottom edges, and
      // starts just inside its left and top ones unless it opens a row or
      // column. The slack is also what keeps rounding from dropping a point
      // that sits on a seam out of both neighbours.
      const QRectF clip =
          region.adjusted(i > i0 ? kGalvoSeamEpsilonMm : -kGalvoSeamEpsilonMm,
                          j > j0 ? kGalvoSeamEpsilonMm : -kGalvoSeamEpsilonMm,
                          kGalvoSeamEpsilonMm, kGalvoSeamEpsilonMm);
      blocks.append({region, clip, park});
    }
  }
  // The lattice is anchored at the machine origin and its cells span the whole
  // work area, so this only bites when the travel keeps the head off the
  // lattice points an edge of the drawing needs. Losing geometry silently would
  // be far worse than saying so.
  QRectF covered;
  for (const GalvoBlock& block : blocks) {
    covered = covered.united(block.region);
  }
  if (!covered.contains(content)) {
    qWarning() << "[Export] galvo blocks cover" << covered << "but the layer needs"
               << content << "-- geometry outside that will not be engraved";
  }
  return blocks;
}

void ToolpathExporterFcode::writeGalvoDebugImage(const QVector<GalvoBlock>& blocks,
                                                 const QRectF& content) const {
  // Drawn in the design frame, the one the work area is in. The gantry's own
  // coordinates are the main head's position in that frame, so a park is
  // plotted as it stands -- but the lens is a module offset away from it, and
  // everything the beam does hangs off the lens, not the head. With a big
  // enough offset the head can even stand outside the area its own beam covers,
  // which is exactly the thing worth being able to see.
  constexpr double kPxPerMm = 2;
  const double half = config_.galvo_field_mm / 2;
  QImage image(qRound(work_area_mm_.width() * kPxPerMm),
               qRound(work_area_mm_.height() * kPxPerMm),
               QImage::Format_ARGB32);
  image.fill(Qt::white);
  QPainter painter(&image);
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.scale(kPxPerMm, kPxPerMm);
  // Widths are in mm because the painter is scaled; one pixel is 1/kPxPerMm mm.
  auto pen = [](QColor color, double width, Qt::PenStyle style = Qt::SolidLine) {
    QPen p(color);
    p.setWidthF(width);
    p.setStyle(style);
    return p;
  };
  const double hairline = 1 / kPxPerMm;

  // Geometry reaches this function in the frame the factory emits, which is the
  // design frame less the module offset; adding it back puts the beam where the
  // drawing actually is.
  const QPointF lens = layer_offset_;

  painter.setBrush(Qt::NoBrush);
  painter.setPen(pen(QColor(170, 170, 170), hairline));
  // inset by half a line so the border is not half outside the image
  painter.drawRect(QRectF(QPointF(0, 0), work_area_mm_)
                       .adjusted(hairline / 2, hairline / 2, -hairline / 2,
                                 -hairline / 2));
  // where the head itself can stand -- a head position, so no offset
  painter.setPen(pen(QColor(70, 110, 200), hairline, Qt::DashLine));
  painter.drawRect(galvoHeadTravel());

  for (const GalvoBlock& block : blocks) {
    const QPointF centre = block.park + lens;
    const QRectF reach(centre.x() - half, centre.y() - half, half * 2,
                       half * 2);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 140, 0, 26));
    painter.drawRect(reach);
    painter.setBrush(Qt::NoBrush);
    painter.setPen(pen(QColor(230, 120, 0, 160), hairline));
    painter.drawRect(reach);
    // the slice of that reach this head is actually responsible for
    painter.setPen(pen(QColor(0, 140, 70), hairline * 2));
    painter.drawRect(block.region.translated(lens));
    // how far the lens sits from the head that carries it
    painter.setPen(pen(QColor(150, 150, 150), hairline));
    painter.drawLine(block.park, centre);
    // and where that head stands to do it
    painter.setPen(pen(QColor(200, 0, 0), hairline * 2));
    painter.drawLine(QPointF(block.park.x() - 3, block.park.y()),
                     QPointF(block.park.x() + 3, block.park.y()));
    painter.drawLine(QPointF(block.park.x(), block.park.y() - 3),
                     QPointF(block.park.x(), block.park.y() + 3));
  }

  painter.setPen(pen(QColor(0, 0, 0), hairline * 2));
  painter.setBrush(Qt::NoBrush);
  painter.drawRect(content.translated(lens));

  painter.resetTransform();
  painter.setFont(QFont(painter.font().family(), 11));
  const QStringList legend = {  // NOLINT
      QString("%1 blocks, field %2 mm, displacement %3 x %4 mm")
          .arg(blocks.size())
          .arg(config_.galvo_field_mm)
          .arg(config_.galvo_block_size.width())
          .arg(config_.galvo_block_size.height()),
      QString("module offset %1, %2 mm: the lens is that far from the head")
          .arg(lens.x())
          .arg(lens.y()),
      "red cross = gantry position   orange = galvo reach from its lens",
      "green = block engraved   black = layer content   blue dash = head travel"};
  // on its own backing, since it sits over the drawing
  int width = 0;
  const QFontMetrics metrics(painter.font());
  for (const QString& line : legend) {
    width = qMax(width, metrics.horizontalAdvance(line));
  }
  painter.setPen(Qt::NoPen);
  painter.setBrush(QColor(255, 255, 255, 225));
  painter.drawRect(QRect(0, 0, width + 16, 15 * legend.size() + 10));
  int y = 18;
  for (const QString& line : legend) {
    painter.setPen(QColor(60, 60, 60));
    painter.drawText(8, y, line);
    y += 15;
  }
  painter.end();

  QString path = config_.galvo_debug_image;
  const int dot = path.lastIndexOf('.');
  const QString suffix = QString("-layer%1").arg(current_layer_id_);
  path = dot > path.lastIndexOf('/') ? path.left(dot) + suffix + path.mid(dot)
                                     : path + suffix + ".png";
  if (image.save(path)) {
    qInfo() << "[Export] galvo tiling image ->" << path;
  } else {
    qWarning() << "[Export] could not write the galvo tiling image to" << path;
  }
}

void ToolpathExporterFcode::convertGalvoLaserLayer() {
  // The gantry only ever positions in a galvo job, so it runs at its own speed
  // for the whole layer. Inside a block this has no effect anyway: those
  // travels are galvo jumps and take SET_JUMP_SPEED instead.
  proc.set_travel_speed(config_.galvo_travel_speed);

  prepareGalvoBitmaps();
  const bool has_hatch =
      laser_hatch_factory_ && !laser_hatch_factory_->is_empty();
  const bool has_raster =
      laser_raster_factory_ && !laser_raster_factory_->is_empty();
  if (laser_path_factory_->get_size() == 0 && !has_hatch && !has_raster) {
    convertGalvoDepthBitmaps();
    proc.set_travel_speed(config_.travel_speed);
    onProgressChanged(1.0, true);
    return;
  }

  GalvoListWriter* galvo = proc.galvo();
  const int out_of_field_before = galvo->out_of_field_count();
  applyGalvoLayerParams();
  const double half_field = config_.galvo_field_mm / 2;
  QRectF content = laser_path_factory_->get_bounds_mm();
  if (has_hatch) {
    content = content.united(laser_hatch_factory_->get_bounds_mm());
  }
  if (has_raster) {
    content = content.united(laser_raster_factory_->get_bounds_mm());
  }
  const QVector<GalvoBlock> blocks = planGalvoBlocks(content);
  qInfo() << "[Export] Galvo layer" << current_layer_->name() << "content"
          << content << "->" << blocks.size() << "block(s), field"
          << config_.galvo_field_mm << "mm, displacement"
          << config_.galvo_block_size;
  if (!config_.galvo_debug_image.isEmpty()) {
    writeGalvoDebugImage(blocks, content);
  }

  convert_target_ = ConvertTarget::NON_BITMAP;
  emitGalvoBlocks(
      blocks,
      [&]() {
        // Not outputLayerPathFcode(): its acceleration override writes gantry
        // commands, and a galvo block has to stay one unbroken run of byte 23
        // (§19.1). Acceleration is a gantry notion anyway.
        laser_path_factory_->generate_task_code(layer_path_speed_);
        if (has_hatch) {
          // Same clip, so the hatch splits across blocks exactly as the
          // outlines do.
          laser_hatch_factory_->generate_task_code(layer_speed_);
        }
        if (has_raster) {
          laser_raster_factory_->generate_task_code(layer_speed_);
        }
      },
      0.05, galvo_depth_bitmaps_.isEmpty() ? 1.0 : 0.5,
      galvo_depth_bitmaps_.isEmpty());
  if (this->cancelled_) {
    return;
  }
  convertGalvoDepthBitmaps();

  proc.set_travel_speed(config_.travel_speed);
  const int out_of_field = galvo->out_of_field_count() - out_of_field_before;
  if (out_of_field > 0) {
    qWarning() << "[Export] layer" << current_layer_->name() << "put"
               << out_of_field << "galvo points outside +/-" << half_field
               << "mm";
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

  // Resolve s-curve parameters for this layer. When a manual override is
  // provided (a_max/jerk > 0) use it, otherwise fall back to the hardware
  // defaults. s_curve_a is used to override the fill acceleration on x.
  std::optional<SCurveParameters> s_curve_params;
  double s_curve_padding = NAN;
  double s_curve_a = NAN;
  if (config_.enable_s_curve && current_layer_->sCurveEnable()) {
    if (current_layer_->sCurveAMax() > 0 && current_layer_->sCurveJerk() > 0) {
      s_curve_params = SCurveParameters{current_layer_->sCurveA0(),
                                        current_layer_->sCurveAMax(),
                                        current_layer_->sCurveJerk()};
    } else {
      s_curve_params = get_s_curve_parameters(hardware_, layer_speed_ / 60.0);
    }
    if (s_curve_params) {
      s_curve_padding = calculate_s_curve_padding_dist(
          layer_speed_ / 60.0, s_curve_params->a0, s_curve_params->a_max,
          s_curve_params->jerk);
      if (s_curve_padding > 0) {
        s_curve_a = floor(pow(layer_speed_ / 60.0, 2) / (2.0 * s_curve_padding));
      }
      qInfo() << "Enable S-Curve a0:" << s_curve_params->a0
              << "a_max:" << s_curve_params->a_max
              << "jerk:" << s_curve_params->jerk << "a:" << s_curve_a
              << "padding:" << s_curve_padding << "mm";
    }
  }

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
  // Override fill acc-x for s-curve
  if (s_curve_params) {
    acc_override_object.is_valid = true;
    acc_override_object.x = s_curve_a;
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
  if (s_curve_params) {
    padding_dist = s_curve_padding;
    qInfo() << "Turn on s-curve";
    // Emits motion params 154/155/157; enable is implied by the params.
    proc.set_s_curve_params(s_curve_params->a0, s_curve_params->a_max,
                            s_curve_params->jerk);
    proc.set_s_curve_enabled(true);
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
  if (s_curve_params) {
    qInfo() << "Turn off s-curve";
    proc.sync_grbl_motion(156);
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
      if (!config_.color_order.isEmpty()) {
        qInfo() << "Color Order:" << config_.color_order;
        factory_->set_color_order(config_.color_order);
      }
      if (hw_profile.reverse_4c) {
        factory_->set_reversed(true);
      }
      if (macros) {
        factory_->set_macros(macros);
      }
      factory_->set_am_angle_map(current_layer_->rawAmAngleMap());
      factory_->set_color_curves_map(current_layer_->rawColorCurvesMap());
      factory_->set_refresh_interval(current_layer_->refreshInterval());
      factory_->set_burst_refresh(config_.burst_refresh);
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
    // layer padding (data-printingTopPadding / data-printingBotPadding) overrides global
    int layer_top_padding = current_layer_->printingTopPadding();
    int layer_bot_padding = current_layer_->printingBotPadding();
    factory_->set_slice_top_padding(layer_top_padding >= 0 ? layer_top_padding : config_.printing_top_padding);
    factory_->set_slice_bot_padding(layer_bot_padding >= 0 ? layer_bot_padding : config_.printing_bot_padding);
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
      if (!config_.skip_prespray && !config_.burst_refresh &&
          hasattr(macros, MacroFunc::test_cartridge)) {
        proc.start_task_script_block("xMIN", "0002");
        qInfo() << "Prespray" << config_.prespray_times << "times";
        macros->test_cartridge(config_.prespray_times);
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
      // 0008: pure reset table
      if (hasattr(macros, MacroFunc::reset_table)) {
        proc.start_task_script_block("xMIN", "0008");
        macros->reset_table();
        macros->post_table_motion();
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
  if (layer_is_galvo_ && convert_target_ == ConvertTarget::BITMAP_ONLY) {
    // A galvo expands its own raster (§19.5), so the image is prepared here and
    // walked later, once per block -- never painted into a workspace.
    if (bmp->depthPass() > 0) {
      // The layer owns these for the whole export, as the gcode path assumes too.
      galvo_depth_bitmaps_.append(bmp);
      return;
    }
    QTransform transform = global_transform_ * laser_raster_factory_->get_transform();
    QRectF bbox_px = transform.mapRect(bmp->boundingRect());
    transform = bmp->transform() * transform;
    QImage image = bmp->sourceImage()
                       .transformed(transform, bmp->gradient()
                                                   ? Qt::SmoothTransformation
                                                   : Qt::FastTransformation)
                       .convertToFormat(QImage::Format_ARGB32);
    bbox_px.setWidth(image.width());
    bbox_px.setHeight(image.height());
    clearTransparent(&image);
    if (bmp->gradient()) {
      // Tone becomes dot density, the way the gcode path does it: dither to one
      // bit, then one dot per surviving pixel.
      texturizeLaserImage(&image, false);
      image = image.convertToFormat(QImage::Format_Mono,
                                    Qt::MonoOnly | Qt::DiffuseAlphaDither)
                  .convertToFormat(QImage::Format_Grayscale8);
    } else {
      image = imageBinarize(&image, bmp->thrsh_brightness());
      dilateBinaryBitmap(&image);
      texturizeLaserImage(&image, true);
    }
    laser_raster_factory_->add_bitmap(image, bbox_px, bmp->gradient());
    return;
  }
  QTransform transform = global_transform_ * factory_->get_transform();
  QRectF new_dirty_area = transform.mapRect(bmp->boundingRect());
  transform = bmp->transform() * transform;
  QImage transformed_image =
      bmp->sourceImage()
          .transformed(transform, bmp->gradient() ? Qt::SmoothTransformation
                                                  : Qt::FastTransformation)
          .convertToFormat(QImage::Format_ARGB32);
  // Note: width and height of new_dirty_area could be floating point, use transformed_image's width and height instead to prevent rounding issue
  new_dirty_area.setWidth(transformed_image.width());
  new_dirty_area.setHeight(transformed_image.height());
  if (bmp->gradient()) {
    if (!is_laser_layer_) {
      if (layer_module_ == LayerModule::PRINTER_4C) {
        factory_->add_image_by_color(transformed_image, new_dirty_area, bmp->color());
      } else {
        factory_->add_image(transformed_image, new_dirty_area);
      }
    } else if (bmp->pwm()) {
      texturizeLaserImage(&transformed_image, false);
      if (!config_.enable_fast_gradient) {
        clearTransparent(&transformed_image);
        transformed_image = transformed_image.convertToFormat(QImage::Format_Mono)
                .convertToFormat(QImage::Format_Grayscale8);
      }
      factory_->add_image(transformed_image, new_dirty_area);
    } else {
      clearTransparent(&transformed_image);
      texturizeLaserImage(&transformed_image, false);
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
    texturizeLaserImage(&transformed_image, true);
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
    if (layer_is_galvo_) {
      // A galvo cannot engrave the bitmap the other machines fill into, so the
      // outline is kept and cut as hatch lines instead (§19.5).
      laser_hatch_factory_->add_filled_path(
          (path->transform() * global_transform_).map(path->path()),
          path->path().fillRule() == Qt::OddEvenFill);
      return;
    }
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
  const RectBorders borders = getRectBorders(dirty_area.intersected(src->rect()));
  for (int y = borders.top; y < borders.bottom_exclusive; ++y) {
    QRgb* ptr = (QRgb*)src->scanLine(y);
    for (int x = borders.left; x < borders.right_exclusive; ++x) {
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

/**
 * Apply the layer's engraving texture in-place. redither converts the textured
 * grays back to a binary image (for images that must stay binarized, e.g.
 * threshold bitmaps and filled paths).
 */
void ToolpathExporterFcode::texturizeLaserImage(QImage* image, bool redither) {
  if (!layer_texture_enabled_ || !image || image->isNull()) {
    return;
  }
  applyLaserTexture(image, layer_texture_params_);
  if (redither) {
    *image = image
                 ->convertToFormat(QImage::Format_Mono,
                                   Qt::MonoOnly | Qt::DiffuseDither)
                 .convertToFormat(QImage::Format_Grayscale8);
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
  NamedArgs args = NamedArgs()
                       .rx(config_.home_pos.x())
                       .ry(config_.home_pos.y())
                       .set_is_travel();
  if (is_galvo_machine_) {
    // The gantry of a galvo machine positions rather than traverses, and this
    // run home is no different -- the ordinary travel speed is too quick for it.
    args.rf(config_.galvo_travel_speed);
  }
  proc.moveto(args);
}

void ToolpathExporterFcode::handleCancel() {
  this->cancelled_ = true;
  if (factory_)
    factory_->handleCancel();
  if (laser_filled_factory_)
    laser_filled_factory_->handleCancel();
  if (laser_path_factory_)
    laser_path_factory_->handleCancel();
  if (laser_hatch_factory_)
    laser_hatch_factory_->handleCancel();
  if (laser_raster_factory_)
    laser_raster_factory_->handleCancel();
  if (laser_depth_factory_)
    laser_depth_factory_->handleCancel();
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
