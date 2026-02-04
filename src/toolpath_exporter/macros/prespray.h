#pragma once

#include "toolpath_exporter/factories/workspace.h"
#include "toolpath_exporter/generators/fcode-generator.h"
#include "toolpath_exporter/toolpath-exporter-types.h"
#include <QMap>
#include <QPointF>
#include <QRectF>

struct PresprayParams {
  QVector<std::shared_ptr<Workspace>>* workspaces;
  QSizeF work_area_mm;
  LayerModule module;
  QRectF prespray;
  InwardRect clip_rect_mm;
  QMap<LayerModule, QPointF> module_offsets = {};
  float travel_speed = 7500;
  float task_speed = 1800;
  bool do_test = false;
  bool has_job_origin = false;
  QPointF job_origin = QPointF(0, 0);
  bool is_rotary_task = false;
  bool rotary_z_motion = false;
  int repeat = 1;
  bool reverse_first = false;
  bool should_enter_printer_mode = true;
  NozzleMode nozzle_mode = NozzleMode::UNDEFINED;
  bool reverse_4c = false;
};

void generate_prespray_code(ToolpathProcessor& proc,
                            const PresprayParams& params);
