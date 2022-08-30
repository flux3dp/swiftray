#pragma once

#include <QList>
#include <QPainter>
#include <QMutex>
#include <QProgressDialog>
#include <layer.h>
#include <shape/bitmap-shape.h>
#include <shape/path-shape.h>
#include <shape/group-shape.h>
#include <toolpath_exporter/generators/base-generator.h>
#include <document.h>
#include <bitset>

#include <QImage>

class ToolpathExporter : public QObject
{
Q_OBJECT

public:
  enum class PaddingType {
      kNoPadding,
      kFixedPadding,  // Padding with a fixed distance
      kDynamicPadding // Based on layer speed and acceleration
  };

  ToolpathExporter(BaseGenerator *generator, qreal dpmm, PaddingType padding) noexcept;

  bool convertStack(const QList<LayerPtr> &layers, bool is_high_speed, QProgressDialog* dialog = nullptr);

  void setWorkAreaSize(QSizeF work_area_size) { machine_work_area_size_ = work_area_size; }

  enum class ScanDirectionMode {
      kBidirectionMode,
      kUnidirectionMode
  };

private:
  void convertLayer(const LayerPtr &layer);

  void convertShape(const ShapePtr &shape);

  void convertGroup(const GroupShape *group);

  void convertBitmap(const BitmapShape *bmp);

  void convertPath(const PathShape *path);

  void sortPolygons();

  void outputLayerGcode();

  void outputLayerPathGcode();

  void outputLayerBitmapGcode();

  inline void moveTo(QPointF&& dest, double speed, double power);
  inline void moveTo(const QPointF& dest, double speed, double power);

  std::tuple<std::vector<std::bitset<32>>, uint32_t, uint32_t> adjustPrefixSuffixZero(
          const std::vector<std::bitset<32>>& src_bit_array, uint32_t padding_dot_cnt);
  bool rasterBitmap(const QImage &layer_image, QRect bbox,
                    ScanDirectionMode direction_mode, qreal padding_mm);
  bool rasterLine(const QLineF& path, const std::vector<std::bitset<32>>& data);
  bool rasterBitmapHighSpeed(const QImage &layer_image, QRect bbox,
                             ScanDirectionMode direction_mode, qreal padding_mm);
  bool rasterLineHighSpeed(const QLineF& path, const std::vector<std::bitset<32>>& data);

  QImage imageBinarize(QImage src, int threshold);

  QTransform global_transform_;
  qreal resolution_scale_; // = dots per unit_size_on_canvas
  QTransform resolution_scale_transform_; // scale matrix of (dpmm_ / canvas_mm_ratio_)
  //QList<ShapePtr> layer_elements_;
  QList<QPolygonF> layer_polygons_; // place the unfilled path geometry
  QMutex polygons_mutex_;
  QPixmap layer_bitmap_;            // place the filled geometry & image (excluding unfilled path)
  LayerPtr current_layer_;
  std::unique_ptr<QPainter> layer_painter_;
  BaseGenerator *gen_;
  qreal dpmm_ = 10;
  QSizeF machine_work_area_size_; // Work area in real world coordinate (in unit of mm)
  QRectF bitmap_dirty_area_;  // Scaled by resolution (expressed in # of dot)
  QSizeF canvas_size_;       // Scaled by resolution (expressed in # of dot)
  const qreal canvas_mm_ratio_ = 10.0; // Currently 10 units in canvas = 1 mm in real world
                                       // TBD: Calculate this ratio by (canvas_size_ / machine_work_area_size_)

  QPointF current_pos_; // in unit of mm
  PaddingType padding_type_ = PaddingType::kNoPadding;
  qreal fixed_padding_mm_ = 10;
  bool is_high_speed_;
};
