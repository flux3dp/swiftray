#pragma once

#include <QList>
#include <QPainterPath>
#include <QPolygonF>
#include <QRectF>

#include "base-factory.h"

/**
 * Vector hatch fill, for filled paths on a galvo head.
 *
 * Everywhere else a filled path is painted into a bitmap workspace and engraved
 * as raster. A galvo cannot take that route -- raster has no galvo path until
 * phase 2 (HX2_GALVO_PROTOCOL.md §19.5) -- so the fill is cut as vectors
 * instead: scan lines at the layer's angle and interval, intersected with the
 * outline, emitted as laser-on segments.
 *
 * The scan-line pass is the one ToolpathExporter::outputLayerFillGcode() runs
 * for Promark, kept deliberately close to it so the two agree. The difference
 * is units: this works in millimetres throughout, where the gcode version works
 * in document dots and divides at the end.
 *
 * Splitting is not this class's business. Segments go out through `proc` like
 * any other geometry, so whatever the galvo writer has been told to clip to
 * applies to them unchanged.
 */
class LaserPathFilledFactory : public BaseFactory {
 public:
  struct FillParams {
    double interval = 0.1;       // mm between scan lines
    double angle = 0;            // degrees
    bool bidirectional = false;  // alternate direction line to line
    int hatch_count = 1;         // 2 crosses the fill at 90 degrees
  };

  explicit LaserPathFilledFactory(const FactoryKwargs& kwargs) noexcept;

  /** `path_mm` is already in work-area mm. */
  void add_filled_path(const QPainterPath& path_mm, bool is_even_odd);
  bool is_empty() const { return filled_.isEmpty(); }
  int get_size() const { return filled_.size(); }
  void set_fill_params(const FillParams& params) { params_ = params; }

  /** Outline bounds in the frame the moves are emitted in (mm, offset applied). */
  QRectF get_bounds_mm() const;

  void generate_task_code(float speed);

 private:
  struct FilledPath {
    QList<QPolygonF> polys;  // mm
    bool is_even_odd = false;
  };
  /** One outline edge, with which way it crosses the scan direction. */
  struct Edge {
    QLineF line;
    bool is_clockwise = false;
  };
  struct EdgeGroup {
    QList<Edge> edges;
    bool is_even_odd = false;
  };

  /** Outline bounds in mm, before the offset. */
  QRectF bounds() const;
  /** Edges of every outline, oriented against `direction`. */
  QList<EdgeGroup> buildEdges(const QPointF& direction) const;
  /**
   * Where one scan line is inside the fill, as on/off point pairs along it.
   * Overlapping outlines are merged so a shared region is cut once.
   */
  QList<QPointF> spansOn(const QLineF& scan_line,
                         const QPointF& inner_start,
                         const QPointF& inner_end,
                         const QList<EdgeGroup>& groups) const;
  void emitSegment(const QPointF& from, const QPointF& to, float speed);

  QList<FilledPath> filled_;
  FillParams params_;
  QRectF clip_mm_;
  float current_pwm_ = 0;
};
