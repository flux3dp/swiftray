#pragma once

#include "workspace.h"
#include "toolpath_exporter/generators/fcode-generator.h"
#include "toolpath_exporter/toolpath-exporter-types.h"
#include <QMap>
#include <QPointF>
#include <QRect>
#include <QSizeF>
#include <QVector>
#include <optional>

enum class UVType {
  WHITE_INK = 2,
  VARNISH = 3,
};

struct HalftoneParams {
  double smoother = 1;
  double density = 2;
  double angle = 75;
  double multiplier = 1;
  QMap<PrintingColor, double> color_multipliers;
};

struct FactoryKwargs {
  ToolpathProcessor* proc;
  std::function<void(double, bool)> onProgressChanged;
  QVector<std::shared_ptr<Workspace>>* workspaces = nullptr;
  int interpolation = 1;
  std::optional<double> pixel_per_mm;
  std::optional<double> pixel_per_mm_x;
  bool one_way = false;
  int halftone = 1;
  HalftoneParams* halftone_params = nullptr;
  QVector<int> color_curve = {};
  QSizeF work_area_mm;
  InwardRect clip_rect_mm;
  QPointF offset;
  bool split_bbox = false;
  int fg_pwm_limit = 0;
};

struct GenerateTaskKwargs {
  bool reverse_y = false;
  float speed = 12000;
  double padding_dist = 10;
  bool support_fast_gradient = false;
  bool mock_fast_gradient = false;
  double pwm_scale = 1;
  double backlash = 0;
  int multipass = 1;
  double black_ratio = 1.0;
  int repeat = 1;
  double padding_dist_left = 0;
  double padding_dist_right = 0;
};

struct PacketData {
  int px_count = 0;
  QRect box;
  QByteArray payload;  // 8 bits: row 1 + row 2... row 8
};

struct PacketData4C {
  QByteArray payload;  // 8 bits (from 1 to 127): row 1 colors[0] + colors[1] +
                       // colors[2] + colors[3] + row 2...
};

class SlicedBox : public QRect {
 public:
  int padding_top;
  QVector<PacketData> data;  // cache, use vector for uv x_step
  PacketData4C data_4c;      // cache
  SlicedBox() : QRect(), padding_top(0) {}
  SlicedBox(QRect rect, int padding = 0) : QRect(rect), padding_top(padding) {}
  SlicedBox(int x, int y, int w, int h, int padding = 0)
      : QRect(x, y, w, h), padding_top(padding) {}
};

using RowBoxes = QVector<SlicedBox>;
using BlockBoxes = QVector<RowBoxes>;