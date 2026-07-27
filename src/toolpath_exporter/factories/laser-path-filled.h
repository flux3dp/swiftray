#pragma once

#include "base-factory.h"
#include "toolpath_exporter/toolpath-utils.h"
#include <QList>
#include <QPainterPath>
#include <QPointF>
#include <QPolygonF>
#include <QRectF>
#include <QVector>

// Promark (galvo) vector hatch fill for filled paths.
//
// Migrated from ToolpathExporterFcode::computeFillBlocks/outputLayerFillFcode:
// filled paths are collected as polygons (work-area mm), a scan-line hatch is
// computed directly in mm (no dpmm scaling), the laser-on segments are split
// across the exporter's blocks (see BaseFactory::set_blocks), and the resulting
// moves are emitted through `proc`. When no blocks are set the fill is emitted
// as a single pass covering its own bounding rect.
class LaserPathFilledFactory : public BaseFactory {
 public:
  struct FillParams {
    double interval = 0.1;       // mm between scan lines
    double angle = 0;            // degrees
    bool bidirectional = false;  // alternate scan direction between lines
    int hatch_count = 1;         // 1, or 2 for cross-hatch
  };

  LaserPathFilledFactory(const FactoryKwargs& kwargs) noexcept;

  // Add a filled path already transformed to work-area mm.
  void add_filled_path(const QPainterPath& path_mm, bool is_even_odd);
  bool is_empty() const { return filled_polygons_.isEmpty(); }
  void set_fill_params(const FillParams& params) { fill_params_ = params; }

  // Compute the hatch segments and emit them, split across the blocks provided
  // via set_blocks() (a single block covering the fill bounds when none set).
  void generate_task_code(float path_speed);

 private:
  struct FilledPath {
    QList<QPolygonF> polys;  // work-area mm
    bool is_even_odd = false;
  };
  struct FillSegment {
    QPointF start;
    QPointF end;
  };
  struct FillBlock {
    QRectF region;  // mm; the head is moved here (block callback) in block mode
    QVector<FillSegment> segments;
  };

  QList<FilledPath> filled_polygons_;
  FillParams fill_params_;
  QSizeF work_area_mm_;
  InwardRect clip_rect_mm_;

  // Scan-line pass: append every laser-on segment (mm) to `segments` and return
  // the fill bounding rect. An empty rect means there is nothing to emit.
  QRectF compute_segments(QVector<FillSegment>& segments);
  // Split `segments` across the emit blocks: block_regions_mm_ when set,
  // otherwise a single block covering `bounds`.
  QVector<FillBlock> split_into_blocks(const QVector<FillSegment>& segments,
                                       const QRectF& bounds) const;
};
