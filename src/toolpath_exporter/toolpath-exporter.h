#pragma once

#include <QLineF>
#include <QList>
#include <QMap>
#include <QPainter>
#include <QMutex>
#include <QProgressDialog>
#include <QImage>

#include <layer.h>
#include <shape/bitmap-shape.h>
#include <shape/path-shape.h>
#include <shape/group-shape.h>
#include <toolpath_exporter/generators/base-generator.h>
#include <document.h>
#include <constants.h>
#include "toolpath-utils.h"
#include "stl-utils.h"
#include "refraction-compensator.h"

struct FilledPath {
  QList<QPolygonF> polys;
  bool isEvenOdd;
};

/**
 * The four kinds of STL object (TODO.md B-2). Each kind needs a different gcode path, and every
 * Z step of a layer emits them in this order -- the enum values double as the index of the per kind
 * slice buckets, so the order lives here and nowhere else.
 */
enum class StlEngraveKind {
  kLineFill = 0,  // contours -> outputLayerFillGcode()
  kLine = 1,      // contours -> outputLayerPathGcode()
  kDotFill = 2,   // contours -> scan lines at the point spacing -> points -> path + dotting time
  kDot = 3,       // contours resampled into points -> outputLayerPathGcode() + dotting time
};
constexpr int kStlEngraveKindCount = 4;

/** An STL object of the canvas, waiting to be sliced at output time. */
struct StlPlacementJob {
  StlPlacement placement;
  /**
   * Resolved while the object's OWN layer was still current_layer_.
   * A canvas layer that mixes filled and unfilled objects reaches the exporter as a pair of layers
   * ("x" and "x-filled"), and the STL output of the pair is deferred to the second one, by which
   * time current_layer_ no longer says whether this object was filled.
   */
  StlEngraveKind kind = StlEngraveKind::kLine;
};

/** The contours one STL object produced at a single Z step of the shared ladder. */
struct StlSlice {
  /** Points into the job list of outputLayerStlGcode(), valid for that layer only. */
  const StlPlacement *placement;
  stl::Layer layer;
};

class ToolpathExporter : public QObject
{
Q_OBJECT

public:
  enum class PaddingType {
      kNoPadding,
      kFixedPadding,  // Padding with a fixed distance
      kDynamicPadding // Based on layer speed and acceleration
  };


  enum BitmapHandlerType {
      NormalMode, // Binary image
      GradientMode, // Gradient image
      PwmMode, // Depth mode image for non-Promark machines; this is not supported in current version
  };

  ToolpathExporter(BaseGenerator *generator, qreal dpmm, double travel_speed, QPointF end_point, PaddingType padding, QTransform move_translate, bool is_promark = false) noexcept;

  bool convertStack(const QList<LayerPtr> &layers, bool is_high_speed, bool start_with_home);

  void setWorkAreaSize(QRectF work_area) {
    machine_work_area_mm_ = work_area; 
    path_utils_.setClipRect(work_area.top() * dpmm_, work_area.right() * dpmm_, work_area.bottom() * dpmm_, work_area.left() * dpmm_);
  }

  bool isExceedingBoundary() { return exceed_boundary_; }

  void setSortRule(PathSort sort_rule) { sort_rule_ = sort_rule; }

  void setLoopCompensation(qreal compensation) {
    compensation_mm_ = compensation;
    path_utils_.setLoopCompensation(compensation / canvas_mm_ratio_ * dpmm_);
  }

  void setEnableCustomBacklash(bool enable) { enable_custom_backlash_ = enable; }

  void handleContour() { is_contour_ = true; }

  /**
   * Meshes of the STL objects of the document, keyed by the id of their placeholder rect.
   * Not owned, must outlive the exporter. Without it every placeholder rect is discarded.
   */
  void setStlObjects(const QMap<QString, stl::Mesh> *objects) { stl_objects_ = objects; }

  enum class ScanDirectionMode {
      kBidirectionMode,
      kUnidirectionMode
  };

  // 
  void parseParam(QJsonObject param);

Q_SIGNALS:
  void progressChanged(int value);

public Q_SLOTS:
  void handleCancel();
 
private:
  void setDpmm(qreal dpmm);

  /**
   * @param stl_paired the layer is the first half of a "x" / "x-filled" pair, so its STL objects
   *        are kept for the second half instead of being engraved now. Without this a canvas layer
   *        that mixes filled and unfilled objects would run the Z ladder twice: the axis would
   *        sweep the whole material, come back down and engrave a second time THROUGH the parts it
   *        just engraved, which is exactly what the deep -> shallow rule (B-5) forbids.
   */
  void convertLayer(const LayerPtr &layer, bool stl_paired = false);

  void convertShape(const ShapePtr &shape);

  void convertGroup(const GroupShape *group);

  void convertBitmap(BitmapShape *bmp);

  void convertPath(const PathShape *path);

  void sortPolygons();

  void outputLayerGcode();

  void outputLayerPathGcode();

  /**
   * Options for outputLayerFillGcode(), used by the STL fill passes.
   * Passing nothing keeps the layer settings and emits gcode as usual.
   */
  struct FillOverride {
    /** false: keep the layer's own scan settings and ignore the four fields below. */
    bool override_scan = false;
    /** Scan line spacing in document dots. <= 0 keeps the layer setting. */
    double interval_dots = 0;
    double angle_deg = 0;
    bool bidirectional = true;
    int hatch_count = 1;
    /**
     * When set, the scan pass emits NO gcode: every filled segment is appended here instead, for
     * the caller to turn into a toolpath of its own.
     */
    QList<QLineF> *out_segments = nullptr;
    /** Skip the per call logging. An STL layer calls this once per Z step, 1500 times over. */
    bool quiet = false;
  };

  void outputLayerFillGcode(const FillOverride *fill_override = nullptr);

  void outputLayerBitmapGcode(BitmapHandlerType type);

  void outputLayerStlGcode();

  StlEngraveKind stlEngraveKind(const StlPlacement &placement, const PathShape *path) const;

  /**
   * Move the head so the nominal focus sits @p focus_z_mm above the focus origin (the platform).
   * Relative move, and the gcode Z of "up" is negative -- see outputLayerStlGcode().
   */
  void moveStlFocusZ(double focus_z_mm);

  /**
   * Straight move in target space, emitted as the laterally compensated (and therefore subdivided)
   * path that actually lands on it.
   */
  void moveStlSegment(const QPointF &from_mm, const QPointF &to_mm, double speed, double power);

  /** One polygon of an STL layer, with the lateral refraction compensation applied. */
  void emitStlPolygon(const QPolygonF &poly_dots, double speed, double power);

  /** One scan line segment of an STL fill, with the lateral refraction compensation applied. */
  void emitStlFillSegment(const QPointF &start_dots, const QPointF &end_dots);

  // The four emitters. Each gets every contour its kind produced at one Z step -- a filled object
  // needs its holes together with its outlines, and objects at the same height share the Z move.
  void outputStlLineFillGcode(const QList<StlSlice> &slices);
  void outputStlLineGcode(const QList<StlSlice> &slices);
  void outputStlDotFillGcode(const QList<StlSlice> &slices);
  void outputStlDotGcode(const QList<StlSlice> &slices);

  inline void moveTo(QPointF&& dest, double speed, double power, double x_backlash);
  inline void moveTo(const QPointF& dest, double speed, double power, double x_backlash);
  int calculatePWMPower(unsigned char grayscale);
  bool rasterBitmap(const QImage &layer_image, QRect bbox,
                    ScanDirectionMode direction_mode, qreal padding_mm, int* count = nullptr,
                    QPointF offset = QPointF(), bool should_transpose = false);
  bool rasterBitmapPwmMode(const QImage &layer_image, QRect bbox,
                    ScanDirectionMode direction_mode, qreal padding_mm);
  bool rasterLine(const QLineF& path, const std::vector<std::array<unsigned char, 32>>& data);
  bool rasterLine(const QLineF& path, const std::vector<std::bitset<32>>& data, bool should_transpose = false);
  bool rasterBitmapHighSpeed(const QImage &layer_image, QRect bbox,
                             ScanDirectionMode direction_mode, qreal padding_mm);
  bool rasterLineHighSpeed(const QLineF& path, const std::vector<std::bitset<32>>& data);
  bool rasterBitmapDepthMode(ScanDirectionMode direction_mode, qreal padding_mm);


  void onProgressChanged(double value, bool absolute);

  bool is_promark_ = false;
  bool is_contour_ = false;
  bool enable_custom_backlash_ = true;
  QTransform global_transform_;
  LayerPtr current_layer_;
  std::unique_ptr<QPainter> layer_painter_;
  BaseGenerator *gen_;
  // === The followings depend on DPI settings of document ===
  qreal dpmm_ = 0;                // The DPMM settings of document
  double travel_speed_ = 80;      // The speed form point to point(mm/s)
  QMutex polygons_mutex_;
  QList<QPolygonF> layer_polygons_; // place the unfilled path geometry, expressed in unit of document dot
  QList<FilledPath> layer_filled_polygons_; // place the filled path geometry, expressed in unit of document dot
  QList<QPixmap> layer_bitmaps_; // place the image according to handler mode, expressed in unit of document dot
  QList<BitmapShape*> depth_mode_bitmaps_; // bitmap shapes for Promark depth mode
  QList<QRectF> bitmap_dirty_areas_;        // Expressed in unit of document dot.
  const QMap<QString, stl::Mesh> *stl_objects_ = nullptr;
  QList<StlPlacementJob> layer_stl_placements_; // STL objects of the current layer, sliced at output time
  /** Set by convertLayer(..., stl_paired): the STL objects belong to the layer that follows. */
  bool stl_output_deferred_ = false;
  /**
   * Refractive index compensation (B-3), configured through parseParam().
   * The depth half is a warp applied to the mesh before slicing, so the ladder still moves Z once
   * per layer; only the (small) lateral half reaches the emitters.
   */
  RefractionCompensator refraction_;
  /** True only while an STL layer whose points need the lateral compensation is being emitted. */
  bool stl_refraction_active_ = false;
  /** The points of layer_polygons_ are dots, not path vertices: nothing may be inserted between. */
  bool stl_dot_mode_ = false;
  /** Height of the nominal focus above the focus origin, mm. Owned by the STL output path. */
  double stl_focus_z_mm_ = 0;
  QSizeF canvas_size_;              // Expressed in unit of document dot.
  QPainterPath canvas_clip_path_;  // Workarea boundary includes a small inward margin to handle floating-point tolerance in contour tasks
  double canvas_width_;
  double canvas_height_;
  // === The followings depend on canvas resolution ===
  const qreal canvas_mm_ratio_ = 10.0; // Currently 10 units in canvas = 1 mm in real world
                                       // TBD: Calculate this ratio by (canvas_size_ / machine_work_area_mm_)
  // == The followings depend on both canvas resolution and document DPI settings ==
  qreal resolution_scale_;                // = dots per unit_size_on_canvas
  QTransform resolution_scale_transform_; // scale matrix of (dpmm_ / canvas_mm_ratio_)
  QTransform move_translate_;             // move matrix
  // === The followings are expressed in unit of mm ===
  QPointF current_pos_mm_; // in unit of mm
  QRectF machine_work_area_mm_; // Work area in real world coordinate (in unit of mm)
  PaddingType padding_type_ = PaddingType::kNoPadding;
  qreal fixed_padding_mm_ = 10;
  QPointF end_point_;
  qreal compensation_mm_ = 0;
  // ==================================================
  bool is_high_speed_ = false;
  bool exceed_boundary_ = false; // Whether source objects exceeding the work area
  bool with_image_ = false;
  bool cancelled_ = false;
  PathSort sort_rule_;
  PathUtils path_utils_;
  // ===== Calculate current progress percentage ======
  int total_layer_cnt_ = 1;
  int processed_layer_cnt_ = 0;
  int total_repeat_times_ = 1;
  int processed_repeat_times_ = 0;
  int element_cnt_[5] = {0, 0, 0, 0, 0}; // 0 Normal Bitmap, 1 Gradient Bitmap, 2 Depth Bitmap(Promark), 3 Filled Path, 4 Unfilled Path
  int total_element_cnt_ = 0;
  double current_progress_ = 0; // progress within current repeat
  int progress_ = 0;
};
