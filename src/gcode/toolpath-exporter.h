#pragma once

#include <QList>
#include <QPainter>
#include <layer.h>
#include <shape/bitmap-shape.h>
#include <shape/path-shape.h>
#include <shape/group-shape.h>
#include <gcode/generators/base-generator.h>
#include <document.h>

#include <QImage>
#include <QTime>

class ToolpathExporter {
public:
  ToolpathExporter(BaseGenerator *generator, qreal dpmm) noexcept;

  void convertStack(const QList<LayerPtr> &layers);

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

  inline void moveTo(QPointF&& dest, int speed, int power);
  inline void moveTo(const QPointF& dest, int speed, int power);

  bool rasterBitmap(const QImage &layer_image, const qreal &mm_per_pixel,
                    const qreal &mm_per_dot, QRectF bbox_mm,
                    ScanDirectionMode direction_mode);
  bool rasterBitmapHighSpeed(const QImage &layer_image, const qreal &mm_per_pixel,
                             const qreal &mm_per_dot, QRectF bbox_mm,
                             ScanDirectionMode direction_mode);
  bool rasterLineHighSpeed(const std::vector<std::bitset<32>>& data);

  QImage imageBinarize(QImage src, int threshold);

  QTransform global_transform_;
  //QList<ShapePtr> layer_elements_;
  QList<QPolygonF> layer_polygons_; // place the unfilled path geometry
  QPixmap layer_bitmap_;            // place the filled geometry & image (excluding unfilled path)
  LayerPtr current_layer_;
  std::unique_ptr<QPainter> layer_painter_;
  BaseGenerator *gen_;
  float dpmm_ = 10;
  QSizeF machine_work_area_size_; // Work area in real world coordinate (in unit of mm)
  QRectF bitmap_dirty_area_; // In canvas unit (not in real world mm unit)
  QSizeF canvas_size_;       // In canvas unit (not in real world mm unit)
  const qreal canvas_mm_ratio_ = 10.0; // Currently 10 unit in canvas = 1 mm in real world
                                       // TBD: Calculate this ratio by (canvas_size_ / machine_work_area_size_)

  QPointF current_pos_; // in unit of mm
  qreal padding_mm_ = 10;
};
