#pragma once

#include <QList>
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

struct FilledPath {
  QList<QPolygonF> polys;
  bool isEvenOdd;
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

  void handleContour() { is_contour_ = true; }

  enum class ScanDirectionMode {
      kBidirectionMode,
      kUnidirectionMode
  };

Q_SIGNALS:
  void progressChanged(int value);

public Q_SLOTS:
  void handleCancel();
 
private:
  void setDpmm(qreal dpmm);

  void convertLayer(const LayerPtr &layer);

  void convertShape(const ShapePtr &shape);

  void convertGroup(const GroupShape *group);

  void convertBitmap(BitmapShape *bmp);

  void convertPath(const PathShape *path);

  void sortPolygons();

  void outputLayerGcode();

  void outputLayerPathGcode();

  void outputLayerFillGcode();

  void outputLayerBitmapGcode(BitmapHandlerType type);

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
