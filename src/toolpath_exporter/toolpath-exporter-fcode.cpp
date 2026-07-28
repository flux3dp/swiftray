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
#include <QLineF>
#include <QtMath>
#include <algorithm>
#include <cmath>
#include <functional>

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
  is_promark_ = param["isPromark"].toBool() || true; // DEV
  qInfo() << "is_promark_" << is_promark_;
  if (SUPPORT_INFO.contains(hardware_)) {
    support_info = SUPPORT_INFO[hardware_];
  }
  QJsonObject workarea = param["workarea"].toObject();
  double width = workarea["width"].toDouble(hw_profile.width);
  double height = workarea["height"].toDouble(hw_profile.length);
  work_area_mm_ = QSizeF(width, height);
  qInfo() << "Canvas size" << work_area_mm_;

  // Optional block_size (mm): when set, the work area is split into blocks of
  // this size for separate processing. Given as [width, height].
  if (param.contains("block_size")) {
    QJsonArray block_size = param["block_size"].toArray();
    if (block_size.size() == 2) {
      config_.block_size =
          QSizeF(block_size[0].toDouble(), block_size[1].toDouble());
    }
  }
  // Optional galvo_size (mm): the galvo addressable field, always larger than
  // block_size. A layer whose dirty area fits within it is emitted in one shot
  // instead of being split into blocks. Given as [width, height].
  if (param.contains("galvo_size")) {
    QJsonArray galvo_size = param["galvo_size"].toArray();
    if (galvo_size.size() == 2) {
      config_.galvo_size =
          QSizeF(galvo_size[0].toDouble(), galvo_size[1].toDouble());
    }
  }
  splitWorkarea();

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
  config_.prespray_times = param["prespray_times"].toInt(3);
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


  // ---- Dev fluence / CO2-tube compensation config -------------------------
  // Ported from the laser-phy-simulator (tools/generate-fcode.ts `Options`
  // defaults + src/core/types.ts `DEFAULT_MACHINE`). These are developer knobs
  // for pre-compensating the tube's power-envelope ramp; edit here to tune. The
  // defaults keep the handler OFF (FluenceStrategy::Baseline) so emission is
  // unchanged until a strategy/lever is enabled.
  dev_fluence_.physics.laser_ramp_up_s = param["laser_ramp_up_s"].toDouble(0.00068);   // laserRampUpS
  dev_fluence_.physics.laser_ramp_down_s = param["laser_ramp_down_s"].toDouble(0.0001);  // laserRampDownS
  // Promark hardware config (formerly PromarkJobConfig in src/constants.h), now
  // dynamically adjustable and pushed to the execution end as {23,8/9/10} cmds.
  dev_fluence_.baseline.jump_speed_mm_s = param["jump_speed_mm_s"].toDouble(4000);     // JUMP_SPEED
  dev_fluence_.baseline.mark_speed_ctrl = param["mark_speed_ctrl"].toDouble(1000);
  dev_fluence_.baseline.jump_delay_min = param["jump_delay_min"].toDouble(200);       // JUMP_DELAY_MIN
  dev_fluence_.baseline.jump_delay_max = param["jump_delay_max"].toDouble(400);       // JUMP_DELAY_MAX
  dev_fluence_.baseline.jump_delay_limit = param["jump_delay_limit"].toDouble(10);
  dev_fluence_.baseline.laser_on_delay_us = param["laser_on_delay_us"].toDouble(-100);   // LASER_ON_DELAY
  dev_fluence_.baseline.laser_off_delay_us = param["laser_off_delay_us"].toDouble(100);   // LASER_OFF_DELAY
  dev_fluence_.baseline.scanner_mark_delay_us = param["scanner_mark_delay_us"].toDouble(100);
  dev_fluence_.baseline.scanner_polygon_delay_us = param["scanner_polygon_delay_us"].toDouble(50);
  dev_fluence_.baseline.z_pulse_per_mm = param["z_pulse_per_mm"].toDouble(1600);      // Z_PULSE_PER_MM
  dev_fluence_.baseline.z_pulse_per_sec = param["z_pulse_per_sec"].toDouble(4800);     // Z_PULSE_PER_SEC
  dev_fluence_.baseline.a_pulse_per_mm = param["a_pulse_per_mm"].toDouble(63);        // A_PULSE_PER_MM
  dev_fluence_.baseline.a_pulse_per_sec = param["a_pulse_per_sec"].toDouble(3200);     // A_PULSE_PER_SEC
  QString strategy_str = param["strategy"].toString();
  if (strategy_str == "RampComp") {
    dev_fluence_.optimization.strategy = FluenceStrategy::RampComp;
  } else if (strategy_str == "Optimized") {
    dev_fluence_.optimization.strategy = FluenceStrategy::Optimized;
  } else {
    dev_fluence_.optimization.strategy = FluenceStrategy::Baseline;
  }
  dev_fluence_.optimization.adaptive = param["adaptive"].toBool(false);    // --adaptive-speed
  dev_fluence_.optimization.carryover = param["carryover"].toBool(false);   // --carryover-aware
  dev_fluence_.optimization.tail_comp = param["tail_comp"].toBool(false);   // --tail-comp
  dev_fluence_.optimization.edge_ext = param["edge_ext"].toBool(false);    // --edge-ext
  dev_fluence_.optimization.edge_ext_mm = param["edge_ext_mm"].toDouble(0.0);   // mm (dev free value)
  dev_fluence_.optimization.cross_pass = param["cross_pass"].toInt(1);      // --cross-pass
  dev_fluence_.optimization.ramp_steps = param["ramp_steps"].toInt(3);      // --ramp-steps
  dev_fluence_.optimization.comp_down = param["comp_down"].toBool(false);   // --comp-down
  dev_fluence_.list.galvo_list_capacity = 8192;  // galvoListCapacity
  bool segmented = dev_fluence_.optimization.adaptive ||
    dev_fluence_.optimization.strategy == FluenceStrategy::RampComp;
  if (segmented) {
    qInfo() << "Force off delay due to segmented mode";
    dev_fluence_.baseline.laser_on_delay_us = 0;
    dev_fluence_.baseline.laser_off_delay_us = 0;
    dev_fluence_.baseline.scanner_mark_delay_us = 0;
    dev_fluence_.baseline.scanner_polygon_delay_us = 0;
  }
  // -------------------------------------------------------------------------

}

void ToolpathExporterFcode::splitWorkarea() {
  // Split the work area into a grid of galvo blocks (Config::block_size, mm).
  //
  // Head positions sit on a grid of block-size multiples starting at (0, 0). The
  // head at (col*block_w, row*block_h) is responsible for a block-sized region
  // (head_block) centred on it, clipped to the work area. The first head (0, 0)
  // therefore owns only the bottom-right quarter of a full block; interior heads
  // own a full block. Adjacent head_blocks tile the work area with no overlap.
  //
  // TODO: apply per-module offsets to `block`; they are per-layer and not known
  // here, so `block` currently mirrors head_block.
  // TODO: emit blocks as a 2D grid for serpentine ordering across rows.
  blocks_.clear();
  if (config_.block_size.isEmpty()) {
    return;
  }
  const double block_w = config_.block_size.width();
  const double block_h = config_.block_size.height();
  const double area_w = work_area_mm_.width();
  const double area_h = work_area_mm_.height();
  // Cover the work area: the last head must reach the far edge. With the first
  // head at 0 covering block/2, `cols` heads reach (cols-1)*block + block/2.
  const int cols = std::max(1, int(std::ceil(area_w / block_w + 0.5)));
  const int rows = std::max(1, int(std::ceil(area_h / block_h + 0.5)));
  const QRectF area(0, 0, area_w, area_h);
  blocks_.reserve(cols * rows);
  for (int row = 0; row < rows; row++) {
    for (int col = 0; col < cols; col++) {
      Block b;
      b.head_pos = QPointF(col * block_w, row * block_h);
      b.head_block = QRectF(b.head_pos.x() - block_w / 2,
                            b.head_pos.y() - block_h / 2, block_w, block_h)
                         .intersected(area);
      if (b.head_block.isEmpty()) {
        continue;  // fully outside the work area
      }
      b.block = b.head_block;  // TODO: apply per-module offsets
      b.row_index = row;
      b.col_index = col;
      blocks_.append(b);
    }
  }
  qInfo() << "[Export] Split work area" << work_area_mm_ << "into"
          << blocks_.size() << "blocks of" << config_.block_size;
}

bool ToolpathExporterFcode::fitsInGalvo(const QSizeF& size) const {
  if (config_.galvo_size.isEmpty()) {
    return false;
  }
  return size.width() <= config_.galvo_size.width() &&
         size.height() <= config_.galvo_size.height();
}

ToolpathExporterFcode::Block ToolpathExporterFcode::makeSingleBlock(
    const QRectF& dirty_area_mm) const {
  Block b;
  b.head_pos = dirty_area_mm.center();
  QRectF full_block(b.head_pos.x() - config_.galvo_size.width() / 2 - 1e-6,
                    b.head_pos.y() - config_.galvo_size.height() / 2 - 1e-6,
                    config_.galvo_size.width() + 2e-6, config_.galvo_size.height() + 2e-6);
  b.head_block = full_block;
  b.block = full_block;
  b.row_index = 0;
  b.col_index = 0;
  return b;
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
    // Block splitting only applies to simple laser tasks (no modules). The
    // layer is always drawn/pre-processed once on the full work area; block
    // splitting only changes how the result is emitted.
    bool use_blocks = !blocks_.isEmpty();
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
      if (use_blocks) {
        emitLaserBlocks();
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
                   (config_.enable_diode && current_layer_->isUseDiode() &&
                    config_.is_diode_one_way_engraving);
  kwargs.fg_pwm_limit = hw_profile.fg_pwm_limit;
  // Block-split raster emission is driven per layer via the factory's
  // set_blocks() (see emitLaserBlocks()), not the factory-internal block_size.

  // Note: convert path without dpmm_x
  laser_path_factory_ = std::make_unique<LaserPathFactory>(kwargs);
  laser_path_factory_->set_loop_compensation(config_.loop_compensation);
  laser_path_factory_->set_use_ga(config_.use_ga_reorder);
  kwargs.pixel_per_mm_x = dpmm_x;
  factory_ = std::make_unique<LaserBitmapFactory>(kwargs);
  laser_filled_factory_ = std::make_unique<LaserBitmapFactory>(kwargs);
  laser_filled_factory_->set_default_workspace(1);
  // Promark: vector hatch fill for filled paths (collected in convertPath).
  laser_filled_path_factory_ = std::make_unique<LaserPathFilledFactory>(kwargs);

  // Dev fluence / CO2-tube compensation: the tube setpoint is the layer power
  // percent; the nominal speed is taken from each factory's generate call.
  double fluence_power = current_layer_->power();
  factory_->set_fluence_config(dev_fluence_);
  factory_->set_fluence_power(fluence_power);
  laser_filled_factory_->set_fluence_config(dev_fluence_);
  laser_filled_factory_->set_fluence_power(fluence_power);
  laser_filled_path_factory_->set_fluence_config(dev_fluence_);
  laser_filled_path_factory_->set_fluence_power(fluence_power);

  setTransform();
  laser_bitmaps_.clear();
  layer_dirty_area_mm_ = QRectF();
  single_block_ = false;
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

  // If the whole layer's dirty area fits within the galvo field, emit it as a
  // single block (one head position) instead of splitting into blocks. The
  // actual block list (grid subset or single block) is resolved and pushed to
  // the raster factories in emitLaserBlocks().
  single_block_ = !config_.block_size.isEmpty() &&
                  !layer_dirty_area_mm_.isEmpty() &&
                  fitsInGalvo(layer_dirty_area_mm_.size());
  if (single_block_) {
    qInfo() << "[Export] Dirty area" << layer_dirty_area_mm_ << "fits galvo"
            << config_.galvo_size << "-> single block (no splitting)";
  }

  element_cnt_[0] = laser_path_factory_->get_size();
  if (laser_filled_factory_->is_workspace_valid()) {
    element_cnt_[1] += 1;
  }
  if (is_promark_ && !laser_filled_path_factory_->is_empty()) {
    // Promark vector hatch fill is emitted from the filled-path factory instead
    // of the filled-path workspace; count it so the progress math has a
    // non-zero denominator for fill-only layers. The scan-line pass runs later,
    // in the factory's generate_task_code().
    element_cnt_[1] += 1;
    laser_filled_path_factory_->set_fill_params(
        {current_layer_->fillInterval(), current_layer_->fillAngle(),
         current_layer_->fillBidirectional(),
         current_layer_->fillHatch() ? 2 : 1});
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
  if (is_promark_) {
    // No blocks set: the factory emits the fill as a single pass.
    laser_filled_path_factory_->generate_task_code(layer_speed_);
  } else {
    outputBitmapFcode();
  }
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

void ToolpathExporterFcode::startPromarkTask(const Block& block) {
  // Initial move for the block: reposition the head to the block's head_pos (the
  // centre of the block region; machine coordinate = work-area coordinate minus
  // the layer offset). The galvo then addresses the field around head_pos.
  proc.moveto(NamedArgs()
                  .rx(block.head_pos.x() - layer_offset_.x())
                  .ry(block.head_pos.y() - layer_offset_.y())
                  .set_is_travel());
  proc.enter_promark_mode();
  // Push the (dynamically adjustable) Promark hardware config, grouped by the
  // execution end's API call groups. 8/9 map to direct lcs_set_* calls, 10's
  // axis ratios are saved on the execution end for MoveAxis + time estimation.
  const FluenceBaselineConfig& bsl = dev_fluence_.baseline;
  proc.set_promark_jump_speed_ctrl(bsl.jump_speed_mm_s);
  // proc.set_promark_mark_speed_ctrl(bsl.mark_speed_ctrl);
  proc.set_promark_delay_mode(bsl.jump_delay_min, bsl.jump_delay_max,
                              bsl.jump_delay_limit);
  proc.set_promark_laser_delays(bsl.laser_on_delay_us, bsl.laser_off_delay_us);
  proc.set_promark_scanner_delays(bsl.scanner_mark_delay_us,
                                  bsl.scanner_polygon_delay_us);
  proc.set_promark_axis_config(1, bsl.z_pulse_per_mm, bsl.z_pulse_per_sec);  // Z
  proc.set_promark_axis_config(0, bsl.a_pulse_per_mm, bsl.a_pulse_per_sec);  // A
  proc.set_promark_block_center(block.head_pos.x() - layer_offset_.x(),
                                block.head_pos.y() - layer_offset_.y());
  proc.set_promark_pulse(current_layer_->frequency(), 1, current_layer_->pulseWidth());
  // TODO: if with wobble set wobble
  // TODO: add a sync_motion?
  qInfo() << "[Export] Move to block" << block.head_block << "head"
          << block.head_pos << "with offset" << layer_offset_;
}

void ToolpathExporterFcode::endPromarkTask() {
  proc.exit_promark_mode();
  // TODO: add a sync_motion?
  proc.sync_grbl_motion(0);
  proc.start_promark_task();
}

void ToolpathExporterFcode::emitLaserBlocks() {
  // The layer has already been drawn and pre-processed once on the full work
  // area by preprocessLaserLayer() (single set of factories/workspaces).
  //
  // Emit order: for each subtype (path -> filled path -> bitmap), for each
  // block. Paths are clipped to each block geometrically here; raster content
  // (filled paths and bitmaps) is emitted block-by-block inside the bitmap
  // factory, driven by the same block list pushed via set_blocks() below, so
  // those subtypes are emitted with a single generate call each.
  if (blocks_.isEmpty()) {
    return;
  }

  // Choose the blocks to emit for this layer. When the layer's dirty area fits
  // the galvo field (single_block_), emit a single block centred on that area
  // instead of iterating the whole grid.
  QVector<Block> layer_blocks;
  if (single_block_) {
    layer_blocks.append(makeSingleBlock(layer_dirty_area_mm_));
  } else {
    layer_blocks = blocks_;
  }

  // Drive the raster (filled paths / bitmaps) block emission from the same
  // blocks: for each block the factory moves the head (startPromarkTask) and
  // engraves only that block's pixels, then exits (endPromarkTask). Capture the
  // block list by value so the callbacks stay valid for the whole call.
  QVector<QRectF> block_regions;
  block_regions.reserve(layer_blocks.size());
  for (const Block& b : layer_blocks) {
    block_regions.append(b.block);
  }
  auto block_start = [this, layer_blocks](int i) {
    startPromarkTask(layer_blocks[i]);
  };
  auto block_end = [this]() { endPromarkTask(); };
  factory_->set_blocks(block_regions, block_start, block_end);
  laser_filled_factory_->set_blocks(block_regions, block_start, block_end);
  laser_filled_path_factory_->set_blocks(block_regions, block_start, block_end);

  // Part 1: path fcode (paths sorted/pre-processed once; clipped per block,
  // with the per-block initial move done inside outputLayerPathFcode()).
  convert_target_ = ConvertTarget::NON_BITMAP;
  for (const Block& block : layer_blocks) {
    outputLayerPathFcode(&block);
    if (this->cancelled_) return;
  }
  onProgressChanged(0.05 + 0.95 * (1.0 / 3.0), true);

  // Part 2: filled path fcode. Promark emits the vector hatch fill (block-by-
  // block via the filled-path factory); otherwise the raster filled path is
  // emitted block-by-block by the bitmap factory (set_blocks above).
  convert_target_ = ConvertTarget::NON_BITMAP;
  if (is_promark_) {
    laser_filled_path_factory_->generate_task_code(layer_speed_);
  } else {
    outputBitmapFcode();
  }
  if (this->cancelled_) return;
  onProgressChanged(0.05 + 0.95 * (2.0 / 3.0), true);

  // Part 3: bitmap fcode (each bitmap drawn once, emitted block-by-block by the
  // factory using the block list from set_blocks above)
  convert_target_ = ConvertTarget::BITMAP_ONLY;
  setTransform();
  for (auto& shape : laser_bitmaps_) {
    convertShape(shape);
    if (this->cancelled_) return;
  }
  onProgressChanged(1.0, true);
}

void ToolpathExporterFcode::outputLayerPathFcode(const Block* block) {
  if (block) {
    laser_path_factory_->set_block_clip(block->block);
  }
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
  // In block mode, restrict this pass to the block's region. Paths are sorted /
  // pre-processed once (on the first generate call) and only the polyline pieces
  // inside the block are emitted; the clip is cleared afterwards.
  if (block) {
    startPromarkTask(*block);
  }
  laser_path_factory_->generate_task_code(layer_path_speed_);
  if (block) {
    laser_path_factory_->clear_block_clip();
    endPromarkTask();
  }
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
      s_curve_params = get_s_curve_parameters(hardware_, layer_speed_ / 60.0,
                                              layer_is_high_quality_);
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
  // In block mode the factory emits the raster block-by-block, using the block
  // list pushed via set_blocks() (see emitLaserBlocks()); nothing block-specific
  // is needed here.
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
        // Track the layer's dirty area (mm) for the galvo single-block decision.
        layer_dirty_area_mm_ = layer_dirty_area_mm_.united(
            (shape->transform() * global_transform_)
                .mapRect(shape->boundingRect()));
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
    // In block mode the factory emits this bitmap block-by-block, using the
    // block list from set_blocks() (see emitLaserBlocks()).
    bool enable_fast_gradient = config_.enable_fast_gradient;
    bool enable_mock_fast_gradient = config_.enable_mock_fast_gradient;
    if (is_promark_) {
      if (bmp->gradient()) {
        proc.set_promark_dotting_time(current_layer_->dottingTime());
      } else {
        config_.enable_fast_gradient = false;
        config_.enable_mock_fast_gradient = false;
      }
    }
    outputBitmapFcode();
    if (is_promark_) {
      proc.set_promark_dotting_time(0);
      config_.enable_fast_gradient = enable_fast_gradient;
      config_.enable_mock_fast_gradient = enable_mock_fast_gradient;
    }
    factory_->get_workspace()->invalidate();
  }
}

void ToolpathExporterFcode::convertPath(const PathShape* path) {
  // Track the layer's dirty area (mm) for the galvo single-block decision.
  layer_dirty_area_mm_ = layer_dirty_area_mm_.united(
      (path->transform() * global_transform_).mapRect(path->path().boundingRect()));
  bool has_filled =
      ((path->isFilled() && current_layer_->type() == Layer::Type::Mixed) ||
       current_layer_->type() == Layer::Type::Fill ||
       current_layer_->type() == Layer::Type::FillLine);
  if (has_filled) {
    if (is_promark_) {
      // Promark: hand the filled path (work-area mm) to the vector hatch-fill
      // factory instead of rasterizing it into the bitmap workspace.
      QPainterPath transformed_path =
          (path->transform() * global_transform_).map(path->path());
      laser_filled_path_factory_->add_filled_path(
          transformed_path, path->path().fillRule() == Qt::OddEvenFill);
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
  if (laser_filled_path_factory_)
    laser_filled_path_factory_->handleCancel();
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
